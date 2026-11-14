#include "minidayz/engine.hpp"
#include "minidayz/player.hpp"
#include "minidayz/world.hpp"
#include "minidayz/npc_system.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/inventory.hpp"
#include "minidayz/equipment.hpp"
#include "minidayz/item_use.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/generated/item_catalog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {

// Construct 2 creates a fresh random world on a new game; it does not use
// SEED_RUN as a numeric world seed. Use the 3DS clock and hardware tick as
// fresh entropy for this native session. This is a per-run seed, not a claim
// that the original browser Math.random() sequence is reproducible.
constexpr std::uint32_t kSaveMagic = 0x4d445a33u;
constexpr std::uint32_t kSaveVersion = 5u;
constexpr const char* kSavePath = "sdmc:/3ds/Minidayz/world.sav";
constexpr const char* kSaveTempPath = "sdmc:/3ds/Minidayz/world.tmp";

struct LegacyInventorySlotV3 {
    std::uint16_t item_id{};
    std::uint16_t count{};
};

struct LegacyInventorySnapshotV3 {
    std::array<LegacyInventorySlotV3, minidayz::items::kMaximumInventorySlotCount> slots{};
    std::uint16_t capacity{static_cast<std::uint16_t>(minidayz::items::kInventorySlotCount)};
    std::uint16_t equipped_backpack{};
};

struct LegacyEquipmentSnapshotV3 {
    std::array<std::uint16_t, minidayz::items::kEquipmentSlotCount> item_ids{};
};

static_assert(sizeof(LegacyInventorySlotV3) == 4, "v3 inventory slot ABI changed");
static_assert(sizeof(LegacyInventorySnapshotV3) == 132, "v3 inventory snapshot ABI changed");
static_assert(sizeof(LegacyEquipmentSnapshotV3) == 14, "v3 equipment snapshot ABI changed");

struct SavedGame {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{kSaveVersion};
    std::uint32_t seed{};
    minidayz::PlayerState player{};
    minidayz::items::InventorySnapshot inventory{};
    minidayz::items::EquipmentSnapshot equipment{};
    minidayz::items::GroundLootSnapshot ground_loot{};
};

minidayz::items::InventorySnapshot migrate_inventory_v3(
    const LegacyInventorySnapshotV3& legacy) noexcept {
    minidayz::items::InventorySnapshot result{};
    result.capacity = legacy.capacity;
    result.equipped_backpack = legacy.equipped_backpack;
    result.equipped_backpack_condition = legacy.equipped_backpack == 0
        ? 100 : minidayz::items::source_default_condition(legacy.equipped_backpack);
    for (std::size_t i = 0; i < legacy.slots.size(); ++i) {
        const auto item_id = legacy.slots[i].item_id;
        const auto condition = item_id == 0
            ? 100 : minidayz::items::source_default_condition(item_id);
        result.slots[i] = {item_id, legacy.slots[i].count, condition};
    }
    return result;
}

minidayz::items::EquipmentSnapshot migrate_equipment_v3(
    const LegacyEquipmentSnapshotV3& legacy) noexcept {
    minidayz::items::EquipmentSnapshot result{};
    result.item_ids = legacy.item_ids;
    for (std::size_t i = 0; i < legacy.item_ids.size(); ++i) {
        result.conditions[i] = legacy.item_ids[i] == 0
            ? 100 : minidayz::items::source_default_condition(legacy.item_ids[i]);
    }
    return result;
}

bool consume_legacy_padding(std::FILE* file, std::size_t bytes) noexcept {
    unsigned char padding[3]{};
    return bytes <= sizeof(padding) &&
        (bytes == 0 || std::fread(padding, 1, bytes, file) == bytes);
}

bool load_saved_game(SavedGame& save) noexcept {
    std::FILE* file = std::fopen(kSavePath, "rb");
    if (!file) return false;
    const bool header_ok =
        std::fread(&save.magic, sizeof(save.magic), 1, file) == 1 &&
        std::fread(&save.version, sizeof(save.version), 1, file) == 1 &&
        std::fread(&save.seed, sizeof(save.seed), 1, file) == 1;
    bool body_ok = false;
    if (header_ok && save.version == 1u) {
        // Migrate the previous player-only save format without discarding it.
        body_ok = std::fread(&save.player, sizeof(save.player), 1, file) == 1;
        save.inventory = {};
        save.equipment = {};
    } else if (header_ok && (save.version == 2u || save.version == 3u)) {
        LegacyInventorySnapshotV3 legacy_inventory{};
        body_ok =
            std::fread(&save.player, sizeof(save.player), 1, file) == 1 &&
            std::fread(&legacy_inventory, sizeof(legacy_inventory), 1, file) == 1;
        if (body_ok) save.inventory = migrate_inventory_v3(legacy_inventory);
        if (body_ok && save.version == 3u) {
            LegacyEquipmentSnapshotV3 legacy_equipment{};
            body_ok = std::fread(&legacy_equipment, sizeof(legacy_equipment), 1, file) == 1;
            if (body_ok) save.equipment = migrate_equipment_v3(legacy_equipment);
        } else {
            save.equipment = {};
        }
        if (body_ok) {
            const std::size_t legacy_bytes = 3 * sizeof(std::uint32_t) +
                sizeof(minidayz::PlayerState) + sizeof(LegacyInventorySnapshotV3) +
                (save.version == 3u ? sizeof(LegacyEquipmentSnapshotV3) : 0);
            const std::size_t padding = (4 - (legacy_bytes % 4)) % 4;
            body_ok = consume_legacy_padding(file, padding);
        }
    } else if (header_ok && save.version == 4u) {
        // Version 4 persisted player, inventory and equipment; ground loot was
        // not part of that format, so it starts empty when migrated.
        body_ok =
            std::fread(&save.player, sizeof(save.player), 1, file) == 1 &&
            std::fread(&save.inventory, sizeof(save.inventory), 1, file) == 1 &&
            std::fread(&save.equipment, sizeof(save.equipment), 1, file) == 1;
        save.ground_loot = {};
    } else if (header_ok && save.version == kSaveVersion) {
        body_ok =
            std::fread(&save.player, sizeof(save.player), 1, file) == 1 &&
            std::fread(&save.inventory, sizeof(save.inventory), 1, file) == 1 &&
            std::fread(&save.equipment, sizeof(save.equipment), 1, file) == 1 &&
            std::fread(&save.ground_loot, sizeof(save.ground_loot), 1, file) == 1;
    }
    const bool no_extra_data = std::fgetc(file) == EOF;
    std::fclose(file);
    return header_ok && body_ok && no_extra_data && save.magic == kSaveMagic &&
        save.seed != 0 &&
        std::isfinite(save.player.x) && std::isfinite(save.player.y) &&
        std::isfinite(save.player.health) && std::isfinite(save.player.hunger) &&
        std::isfinite(save.player.thirst) && save.player.x >= 0.0f &&
        save.player.y >= 0.0f &&
        save.player.x <= minidayz::WorldSystem::kSourceMapWidth &&
        save.player.y <= minidayz::WorldSystem::kSourceMapHeight;
}

bool write_saved_game(std::uint32_t seed,
                      const minidayz::PlayerState& player,
                      const minidayz::items::InventorySystem& inventory,
                      const minidayz::items::EquipmentSystem& equipment,
                      const minidayz::items::GroundLootSystem& ground_loot) noexcept {
    (void)::mkdir("sdmc:/3ds", 0777);
    (void)::mkdir("sdmc:/3ds/Minidayz", 0777);
    SavedGame save{};
    save.seed = seed;
    save.player = player;
    save.inventory = inventory.snapshot();
    save.equipment = equipment.snapshot();
    save.ground_loot = ground_loot.snapshot();
    std::FILE* file = std::fopen(kSaveTempPath, "wb");
    if (!file) return false;
    const bool written = std::fwrite(&save, sizeof(save), 1, file) == 1 &&
                         std::fflush(file) == 0;
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed) {
        (void)std::remove(kSaveTempPath);
        return false;
    }
    // POSIX rename replaces an existing destination atomically when both
    // paths are on the same filesystem. Do not remove the last good save first:
    // a power loss or failed rename must leave Continue usable.
    if (std::rename(kSaveTempPath, kSavePath) != 0) {
        (void)std::remove(kSaveTempPath);
        return false;
    }
    return true;
}

std::uint32_t make_new_world_seed() noexcept {
    const std::uint64_t wall = static_cast<std::uint64_t>(osGetTime());
    const std::uint64_t ticks = static_cast<std::uint64_t>(svcGetSystemTick());
    std::uint64_t value = wall ^ (ticks + 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    value ^= value >> 31;
    const std::uint32_t seed = static_cast<std::uint32_t>(value ^ (value >> 32));
    return seed != 0 ? seed : 0x6d696e69u;
}

class WorldPreviewScene final : public minidayz::Scene {
public:
    explicit WorldPreviewScene(const SavedGame* restored = nullptr) noexcept
        : world_seed_(restored ? restored->seed : make_new_world_seed()),
          has_saved_game_(restored != nullptr),
          player_(restored ? restored->player : make_initial_state()) {
        if (restored && !inventory_.restore(restored->inventory))
            inventory_.clear();
        if (restored && !equipment_.restore(restored->equipment))
            equipment_.clear();
        if (restored && !ground_loot_.restore(restored->ground_loot))
            ground_loot_.clear();
        // Preview-only movement tuning. Source-equivalent values still need
        // to be supplied by the gameplay rules port.
        // Temporary preview movement tuning only. Original source movement
        // values are not guessed; they will replace these when the movement
        // event/ACE trace is complete.
        preview_rules_.walk_speed = 72.0f;
        preview_rules_.run_speed = 108.0f;
        preview_rules_.collision_half_width = 5.0f;
        preview_rules_.collision_half_height = 7.0f;
        preview_rules_.max_health = 100.0f;
        preview_rules_.max_hunger = 100.0f;
        preview_rules_.max_thirst = 100.0f;
        preview_rules_.min_temperature = -10.0f;
        preview_rules_.max_temperature = 10.0f;
        world_ready_ = world_.generate_from_source(world_seed_, "Map", 0);
        if (world_ready_) npc_ready_ = npcs_.initialize(world_);
    }

    void on_exit() noexcept override {
        if (world_ready_ && !menu_active_)
            (void)write_saved_game(world_seed_, player_.state(), inventory_, equipment_, ground_loot_);
    }

    void update(const minidayz::InputState& input, float dt) noexcept override {
        if (menu_active_) {
            bool continue_game = has_saved_game_ && (input.pressed & KEY_A) != 0;
            bool new_game = (input.pressed & KEY_X) != 0;
            touch_down_ = input.touching;
            if (input.touching) {
                const int tx = input.touch.px;
                const int ty = input.touch.py;
                if (tx >= 18 && tx <= 302 && ty >= 78 && ty <= 132)
                    continue_game = has_saved_game_;
                if (tx >= 18 && tx <= 302 && ty >= 145 && ty <= 202)
                    new_game = true;
            }
            if (new_game) start_new_game();
            else if (continue_game) menu_active_ = false;
            else if (!has_saved_game_ && (input.pressed & KEY_A))
                start_new_game();
            return;
        }
        if (selected_inventory_slot_ >= 0 && (input.pressed & (KEY_A | KEY_X))) {
            (void)minidayz::items::use_item_from_inventory(
                inventory_, static_cast<std::size_t>(selected_inventory_slot_),
                player_, preview_rules_, (input.pressed & KEY_X) != 0);
            const auto* selected = inventory_.slot(
                static_cast<std::size_t>(selected_inventory_slot_));
            if (!selected || selected->empty()) selected_inventory_slot_ = -1;
        }
        // L drops the selected stack at the player's feet; R picks up the
        // nearest ground stack. Ground transfers are atomic and persisted.
        if ((input.pressed & KEY_L) && selected_inventory_slot_ >= 0) {
            if (ground_loot_.drop_from_inventory(
                    inventory_, static_cast<std::size_t>(selected_inventory_slot_),
                    0, player_.state().x, player_.state().y))
                selected_inventory_slot_ = -1;
        }
        if (input.pressed & KEY_R)
            (void)ground_loot_.pickup_nearest(
                inventory_, player_.state().x, player_.state().y, 36.0f);
        minidayz::PlayerInput movement{};
        movement.move_x = static_cast<float>(input.circle.dx) / 156.0f;
        movement.move_y = -static_cast<float>(input.circle.dy) / 156.0f;
        if (input.held & KEY_DLEFT)  movement.move_x -= 1.0f;
        if (input.held & KEY_DRIGHT) movement.move_x += 1.0f;
        if (input.held & KEY_DUP)    movement.move_y -= 1.0f;
        if (input.held & KEY_DDOWN)  movement.move_y += 1.0f;
        movement.run = (input.held & KEY_B) != 0;
        const bool touch_pressed = input.touching && !touch_down_;
        const bool touch_released = !input.touching && touch_down_;
        if (touch_pressed) {
            drag_source_slot_ = -1;
            drag_target_slot_ = -1;
            drag_target_equipment_slot_ = -1;
            const int equipment_target = equipment_slot_at(input.touch.px, input.touch.py);
            if (equipment_target >= 0) {
                if (equipment_target == 5) {
                    const auto backpack = inventory_.equipped_backpack();
                    const std::size_t destination = first_available_inventory_slot(backpack);
                    if (backpack != 0 && destination < inventory_.capacity())
                        (void)inventory_.unequip_backpack_to_slot(destination);
                    selected_inventory_slot_ = -1;
                } else {
                    const auto slot = static_cast<minidayz::items::EquipmentSlot>(equipment_target);
                    const auto equipped = equipment_.equipped(slot);
                    if (equipped != 0) {
                        const std::size_t destination = first_available_inventory_slot(equipped);
                        if (destination < inventory_.capacity())
                            (void)equipment_.unequip_to_inventory(inventory_, slot, destination);
                        selected_inventory_slot_ = -1;
                    }
                }
            }
            // Page through larger backpacks without changing the 5 x 3 grid.
            if (input.touch.py >= 223) {
                const std::size_t page_count =
                    (inventory_.capacity() + kInventoryPageSize - 1) /
                    kInventoryPageSize;
                if (input.touch.px < 160 && inventory_page_ > 0)
                    --inventory_page_;
                else if (input.touch.px >= 160 &&
                         inventory_page_ + 1 < page_count)
                    ++inventory_page_;
                selected_inventory_slot_ = -1;
            } else {
                const int slot = inventory_slot_at(input.touch.px,
                                                   input.touch.py,
                                                   inventory_page_);
                if (slot >= 0) {
                    const auto* item = inventory_.slot(static_cast<std::size_t>(slot));
                    selected_inventory_slot_ = item && !item->empty() ? slot : -1;
                    if (selected_inventory_slot_ >= 0) {
                        drag_source_slot_ = slot;
                        drag_target_slot_ = slot;
                    }
                }
            }
        }
        if (input.touching && drag_source_slot_ >= 0) {
            const int equipment_target = equipment_slot_at(input.touch.px, input.touch.py);
            if (equipment_target >= 0 && equipment_target < 6) {
                drag_target_equipment_slot_ = equipment_target;
                drag_target_slot_ = -1;
            } else {
                drag_target_equipment_slot_ = -1;
                const int target = inventory_slot_at(input.touch.px, input.touch.py,
                                                    inventory_page_);
                if (target >= 0) drag_target_slot_ = target;
            }
        }
        if (touch_released) {
            if (drag_source_slot_ >= 0 && drag_target_equipment_slot_ == 5) {
                const auto* dragged = inventory_.slot(
                    static_cast<std::size_t>(drag_source_slot_));
                const std::size_t capacity = dragged
                    ? minidayz::items::source_backpack_capacity(dragged->item_id) : 0;
                if (capacity > minidayz::items::kInventorySlotCount &&
                    inventory_.equip_backpack_from_slot(
                        static_cast<std::size_t>(drag_source_slot_), capacity)) {
                    selected_inventory_slot_ = -1;
                }
            } else if (drag_source_slot_ >= 0 && drag_target_equipment_slot_ >= 0) {
                if (equipment_.equip_from_inventory(
                    inventory_, static_cast<std::size_t>(drag_source_slot_),
                    static_cast<minidayz::items::EquipmentSlot>(drag_target_equipment_slot_)))
                    selected_inventory_slot_ = -1;
            } else if (drag_source_slot_ >= 0 && drag_target_slot_ >= 0 &&
                       drag_target_slot_ != drag_source_slot_) {
                if (inventory_.move(static_cast<std::size_t>(drag_source_slot_),
                                    static_cast<std::size_t>(drag_target_slot_)))
                    selected_inventory_slot_ = drag_target_slot_;
            }
            drag_source_slot_ = -1;
            drag_target_slot_ = -1;
            drag_target_equipment_slot_ = -1;
        }
        touch_down_ = input.touching;
        // A backpack can shrink the inventory back to its 15-slot base. Keep
        // the visible page and selection inside the new capacity immediately.
        const std::size_t valid_page_count =
            (inventory_.capacity() + kInventoryPageSize - 1) / kInventoryPageSize;
        if (inventory_page_ >= valid_page_count)
            inventory_page_ = valid_page_count - 1;
        if (selected_inventory_slot_ >= static_cast<int>(inventory_.capacity()))
            selected_inventory_slot_ = -1;
        if (drag_source_slot_ >= static_cast<int>(inventory_.capacity())) {
            drag_source_slot_ = -1;
            drag_target_slot_ = -1;
            drag_target_equipment_slot_ = -1;
        }
        player_.update_movement(movement, dt, preview_rules_,
                                &WorldPreviewScene::blocked, this);
        // Source-derived survival rates are still zero until their original
        // event cadence is resolved; the update path is now live and ready.
        player_.update_survival(dt, preview_rules_);
        if (npc_ready_) npcs_.update(dt, player_.state().x, player_.state().y,
                                     &WorldPreviewScene::blocked, this);
        save_elapsed_ += dt;
        if (save_elapsed_ >= 2.0f) {
            save_elapsed_ = 0.0f;
            (void)write_saved_game(world_seed_, player_.state(), inventory_, equipment_, ground_loot_);
        }
    }

    void render_bottom(minidayz::Renderer& renderer) noexcept override {
        if (menu_active_) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255));
            renderer.draw_text("WORLD SAVE", 18.0f, 20.0f, 0.55f);
            if (has_saved_game_) {
                renderer.draw_solid_rect(18.0f, 78.0f, 284.0f, 54.0f,
                                         C2D_Color32(62, 92, 66, 255));
                renderer.draw_text("CONTINUE", 92.0f, 96.0f, 0.55f);
            } else {
                renderer.draw_solid_rect(18.0f, 78.0f, 284.0f, 54.0f,
                                         C2D_Color32(55, 65, 58, 255));
                renderer.draw_text("NO SAVE FOUND", 70.0f, 96.0f, 0.45f);
            }
            renderer.draw_solid_rect(18.0f, 145.0f, 284.0f, 57.0f,
                                     C2D_Color32(110, 75, 49, 255));
            renderer.draw_text("NEW GAME", 92.0f, 165.0f, 0.55f);
            if (!world_ready_)
                renderer.draw_text("WORLD GENERATION FAILED - RETRY NEW GAME", 18.0f, 204.0f, 0.27f);
            renderer.draw_text("A: Continue   X: New Game", 18.0f, 216.0f, 0.32f);
            return;
        }
        // The lower 3DS screen is the touch inventory, not a debug panel.
        renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f, C2D_Color32(20, 25, 23, 255));
        renderer.draw_solid_rect(6.0f, 6.0f, 308.0f, 25.0f, C2D_Color32(54, 68, 55, 255));
        // Compact live player status meters. Values are normalized against
        // the preview caps above; exact source caps/rates remain under audit.
        const auto meter = [&](float x, float value, float maximum, u32 color) {
            renderer.draw_solid_rect(x, 15.0f, 68.0f, 8.0f,
                                     C2D_Color32(8, 12, 10, 255));
            const float ratio = maximum > 0.0f
                ? std::max(0.0f, std::min(1.0f, value / maximum)) : 0.0f;
            renderer.draw_solid_rect(x + 1.0f, 16.0f, 66.0f * ratio, 6.0f, color);
        };
        const auto& stats = player_.state();
        meter(10.0f, stats.health, preview_rules_.max_health,
              C2D_Color32(190, 65, 55, 255));
        meter(88.0f, stats.hunger, preview_rules_.max_hunger,
              C2D_Color32(204, 151, 65, 255));
        meter(166.0f, stats.thirst, preview_rules_.max_thirst,
              C2D_Color32(65, 145, 205, 255));
        const float temperature_ratio = (stats.temperature -
            preview_rules_.min_temperature) /
            (preview_rules_.max_temperature - preview_rules_.min_temperature);
        meter(244.0f, temperature_ratio * 100.0f, 100.0f,
              C2D_Color32(75, 180, 165, 255));
        // Original equipment categories: helmet, outerwear, vest, pants,
        // held weapon, plus the separate backpack slot.
        static constexpr const char* equipment_labels[] = {"HELMET","COAT","VEST","PANTS","HANDS","PACK"};
        for (std::size_t i = 0; i < 6; ++i) {
            const float x = 5.0f + static_cast<float>(i) * 52.0f;
            renderer.draw_solid_rect(x, 38.0f, 50.0f, 43.0f, C2D_Color32(8, 11, 10, 255));
            renderer.draw_solid_rect(x + 2.0f, 40.0f, 46.0f, 39.0f, C2D_Color32(47, 54, 48, 255));
            const auto item_id = i == 5
                ? inventory_.equipped_backpack()
                : equipment_.equipped(static_cast<minidayz::items::EquipmentSlot>(i));
            const auto condition = i == 5
                ? inventory_.equipped_backpack_condition()
                : equipment_.snapshot().conditions[i];
            if (item_id != 0) {
                (void)renderer.draw_item_icon(item_id, x + 25.0f, 55.0f, 34.0f, 25.0f);
                if (condition > 0 && condition < 100) {
                    renderer.draw_solid_rect(x + 5.0f, 77.0f, 40.0f, 2.0f,
                                             C2D_Color32(30, 30, 30, 255));
                    renderer.draw_solid_rect(x + 5.0f, 77.0f,
                        40.0f * static_cast<float>(condition) / 100.0f, 2.0f,
                        C2D_Color32(190, 170, 85, 255));
                }
            } else {
                renderer.draw_text(equipment_labels[i], x + 3.0f, 53.0f, 0.18f);
            }
        }
        // 5 x 3 backpack page, sized for the 320 x 240 touch screen.
        const int page_start = static_cast<int>(inventory_page_ * kInventoryPageSize);
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 5; ++col) {
                const int index = page_start + row * 5 + col;
                const float x = 10.0f + col * 60.0f;
                const float y = 92.0f + row * 43.0f;
                const bool selected = index == selected_inventory_slot_;
                renderer.draw_solid_rect(x, y, 56.0f, 39.0f,
                    selected ? C2D_Color32(225, 190, 105, 255) : C2D_Color32(5, 8, 7, 255));
                renderer.draw_solid_rect(x + 2.0f, y + 2.0f, 52.0f, 35.0f,
                    selected ? C2D_Color32(76, 76, 54, 255) : C2D_Color32(43, 49, 44, 255));
                const auto* item = inventory_.slot(static_cast<std::size_t>(index));
                if (item && !item->empty()) {
                    // Use the exact item ID -> Construct object mapping from
                    // the original game's MDZItemTable.
                    (void)renderer.draw_item_icon(item->item_id,
                        x + 28.0f, y + 16.0f, 34.0f, 25.0f);
                    char quantity[8]{};
                    std::snprintf(quantity, sizeof(quantity), "x%u",
                                  static_cast<unsigned>(item->count));
                    renderer.draw_text(quantity, x + 4.0f, y + 27.0f, 0.20f);
                    if (item->condition > 0 && item->condition < 100) {
                        renderer.draw_solid_rect(x + 6.0f, y + 35.0f, 44.0f, 2.0f,
                                                 C2D_Color32(30, 30, 30, 255));
                        renderer.draw_solid_rect(x + 6.0f, y + 35.0f,
                            44.0f * static_cast<float>(item->condition) / 100.0f, 2.0f,
                            C2D_Color32(190, 170, 85, 255));
                    }
                }
            }
        }
        const std::size_t page_count =
            (inventory_.capacity() + kInventoryPageSize - 1) / kInventoryPageSize;
        char page_label[24]{};
        std::snprintf(page_label, sizeof(page_label), "<  BACKPACK  %u/%u  >",
            static_cast<unsigned>(inventory_page_ + 1),
            static_cast<unsigned>(page_count));
        renderer.draw_solid_rect(10.0f, 225.0f, 300.0f, 13.0f,
                                 C2D_Color32(54, 68, 55, 255));
        renderer.draw_text("A USE X ALT L DROP R PICK", 14.0f, 226.0f, 0.18f);
        renderer.draw_text(page_label, 104.0f, 226.0f, 0.30f);
    }

    void render(minidayz::Renderer& renderer) noexcept override {
        if (menu_active_) {
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f, 0.0f, 400.0f, 240.0f,
                                     C2D_Color32(24, 34, 29, 255));
            renderer.draw_solid_rect(30.0f, 42.0f, 340.0f, 3.0f,
                                     C2D_Color32(132, 150, 116, 255));
            renderer.draw_text("MINI DAYZ", 76.0f, 72.0f, 1.05f);
            renderer.draw_text("SURVIVE THE WILDERNESS", 65.0f, 124.0f, 0.42f);
            renderer.draw_text("SELECT CONTINUE OR START A NEW WORLD", 30.0f, 174.0f, 0.32f);
            return;
        }
        const auto& p = player_.state();
        constexpr float view_w = 400.0f;
        constexpr float view_h = 240.0f;
        const float camera_x = std::clamp(p.x - view_w * 0.5f, 0.0f,
            std::max(0.0f, world_.bounds().width - view_w));
        const float camera_y = std::clamp(p.y - view_h * 0.5f, 0.0f,
            std::max(0.0f, world_.bounds().height - view_h));
        renderer.set_camera(camera_x, camera_y);
        renderer.draw_solid_rect(camera_x, camera_y, view_w, view_h,
                                 C2D_Color32(35, 49, 42, 255));

        if (world_ready_) {
            // Render the actual source Map layout in world coordinates.
            // Cull placements outside the camera rectangle to keep 3DS draw
            // cost bounded instead of drawing the entire 36k x 20k layout.
            for (std::size_t i = 0; i < world_.placement_count(); ++i) {
                const auto* placement = world_.placement(i);
                if (!placement) continue;
                const float px = placement->x;
                const float py = placement->y;
                if (px < camera_x - 96.0f || px > camera_x + view_w + 96.0f ||
                    py < camera_y - 96.0f || py > camera_y + view_h + 96.0f)
                    continue;
                const float sx = placement->scale_x > 0.0f ? placement->scale_x : 1.0f;
                if (!renderer.draw_source_sprite(placement->object, 0, 0,
                        px, py, sx, sx, 0.5f,
                        placement->angle * 0.01745329251994329577f)) {
                    // Keep missing atlas frames visible as small source-position
                    // markers, without inventing replacement artwork.
                    renderer.draw_solid_rect(px - 1.0f, py - 1.0f, 2.0f, 2.0f,
                                             C2D_Color32(105, 120, 95, 255));
                }
            }


        }

        for (std::size_t i = 0; i < ground_loot_.count(); ++i) {
            const auto* loot = ground_loot_.entry(i);
            if (!loot || loot->x < camera_x - 24.0f ||
                loot->x > camera_x + view_w + 24.0f ||
                loot->y < camera_y - 24.0f ||
                loot->y > camera_y + view_h + 24.0f) continue;
            (void)renderer.draw_item_icon(loot->item.item_id,
                loot->x, loot->y, 18.0f, 18.0f);
        }

        if (npc_ready_) {
            for (std::size_t i = 0; i < npcs_.count(); ++i) {
                const auto* npc = npcs_.actor(i);
                if (!npc || !npc->alive ||
                    npc->x < camera_x - 48.0f || npc->x > camera_x + view_w + 48.0f ||
                    npc->y < camera_y - 48.0f || npc->y > camera_y + view_h + 48.0f)
                    continue;
                if (!renderer.draw_source_sprite(npc->source_object,
                        npc->animation_index, npc->animation_frame,
                        npc->x, npc->y, 0.55f, 0.55f))
                    renderer.draw_solid_rect(npc->x - 5.0f, npc->y - 5.0f,
                        10.0f, 12.0f, C2D_Color32(130, 145, 78, 255));
                // The original NPC is composed from a skin sprite and a
                // separate eyes sprite pinned to the same position.
                (void)renderer.draw_source_sprite(npc->eyes_object, 0, 0,
                    npc->x, npc->y, 0.55f, 0.55f);
            }
        }

        const bool sprite_drawn = p.alive &&
            renderer.draw_source_sprite(minidayz::kSourcePlayerObject,
                player_.source_animation_index(), player_.source_animation_frame(),
                p.x, p.y, 0.55f, 0.55f);
        if (p.alive) {
            const char* direction = "down";
            switch (p.facing) {
            case minidayz::PlayerFacing::down: direction = "down"; break;
            case minidayz::PlayerFacing::up: direction = "up"; break;
            case minidayz::PlayerFacing::left: direction = "left"; break;
            case minidayz::PlayerFacing::right: direction = "right"; break;
            }
            char animation_name[24]{};
            std::snprintf(animation_name, sizeof(animation_name), "%s_%s",
                p.animation == minidayz::PlayerAnimation::idle ? "idle" : "run",
                direction);
            const auto draw_equipped_layer = [&](std::uint16_t item_id) {
                if (item_id == 0) return;
                const auto object = minidayz::items::source_sprite_object(item_id);
                if (object == 65535) return;
                (void)renderer.draw_source_animation(object, animation_name,
                    p.animation_frame, p.x, p.y, 0.55f, 0.55f, 0.42f);
            };
            // Match the original player equipment layers: pants, clothing,
            // vest, helmet, backpack, then the held weapon.
            draw_equipped_layer(equipment_.equipped(minidayz::items::EquipmentSlot::pants));
            draw_equipped_layer(equipment_.equipped(minidayz::items::EquipmentSlot::outerwear));
            draw_equipped_layer(equipment_.equipped(minidayz::items::EquipmentSlot::vest));
            draw_equipped_layer(equipment_.equipped(minidayz::items::EquipmentSlot::head));
            draw_equipped_layer(inventory_.equipped_backpack());
            const auto held_weapon = equipment_.equipped(minidayz::items::EquipmentSlot::hands);
            if (held_weapon != 0) {
                const auto object = minidayz::items::source_sprite_object(held_weapon);
                if (object != 65535) {
                    char weapon_animation[24]{};
                    const char* candidates[] = {"pistol", "axe", "twohand", "idle", "run"};
                    for (const char* prefix : candidates) {
                        std::snprintf(weapon_animation, sizeof(weapon_animation), "%s_%s",
                                      prefix, direction);
                        if (renderer.draw_source_animation(object, weapon_animation,
                                p.animation_frame, p.x, p.y, 0.55f, 0.55f, 0.42f))
                            break;
                    }
                }
            }
        }
        if (!sprite_drawn) {
            renderer.draw_solid_rect(p.x - 5.0f, p.y - 5.0f, 10.0f, 12.0f,
                                     p.alive ? C2D_Color32(190, 126, 65, 255)
                                             : C2D_Color32(130, 45, 45, 255));
        }
    }

private:
    static constexpr std::size_t kInventoryPageSize = 15;

    static int equipment_slot_at(int x, int y) noexcept {
        if (y < 38 || y >= 81 || x < 5 || x >= 315) return -1;
        const int index = (x - 5) / 52;
        return index >= 0 && index < 6 ? index : -1;
    }

    std::size_t first_available_inventory_slot(std::uint16_t item_id) const noexcept {
        const auto limit = minidayz::items::source_max_stack(item_id);
        for (std::size_t i = 0; i < inventory_.capacity(); ++i) {
            const auto* item = inventory_.slot(i);
            if (!item || item->empty()) return i;
            if (item->item_id == item_id && item->count < limit) return i;
        }
        return inventory_.capacity();
    }

    static int inventory_slot_at(int x, int y, std::size_t page) noexcept {
        if (x < 10 || x >= 310 || y < 92 || y >= 221) return -1;
        const int col = (x - 10) / 60;
        const int row = (y - 92) / 43;
        return col >= 0 && col < 5 && row >= 0 && row < 3
            ? static_cast<int>(page * kInventoryPageSize) + row * 5 + col
            : -1;
    }

    void start_new_game() noexcept {
        world_seed_ = make_new_world_seed();
        player_ = minidayz::PlayerSystem(make_initial_state());
        inventory_.clear();
        equipment_.clear();
        ground_loot_.clear();
        inventory_page_ = 0;
        selected_inventory_slot_ = -1;
        world_ready_ = world_.generate_from_source(world_seed_, "Map", 0);
        npc_ready_ = world_ready_ && npcs_.initialize(world_);
        save_elapsed_ = 0.0f;
        has_saved_game_ = false;
        // Never enter a blank gameplay scene when source world generation
        // fails. Keep the menu available so the player can retry instead.
        menu_active_ = !world_ready_;
        if (world_ready_) (void)write_saved_game(world_seed_, player_.state(), inventory_, equipment_, ground_loot_);
    }

    static minidayz::PlayerState make_initial_state() noexcept {
        minidayz::PlayerState state{};
        state.x = 160.0f;
        state.y = 120.0f;
        state.health = 100.0f;
        state.hunger = 100.0f;
        state.thirst = 100.0f;
        state.alive = true;
        return state;
    }

    static bool blocked(void* context, float x, float y) noexcept {
        const auto* scene = static_cast<const WorldPreviewScene*>(context);
        if (!scene || x < 0.0f || y < 0.0f ||
            x > scene->world_.bounds().width || y > scene->world_.bounds().height)
            return true;
        const auto* map_layout = minidayz::stage1::find_layout("Map");
        if (!map_layout) return true;
        const auto map_index = static_cast<std::uint16_t>(
            map_layout - minidayz::stage1::kLayoutTable);
        for (std::size_t i = 0; i < scene->world_.feature_count(); ++i) {
            const auto* feature = scene->world_.feature(i);
            if (!feature) continue;
            bool blocks = feature->kind == minidayz::WorldFeatureKind::Bunker ||
                          feature->kind == minidayz::WorldFeatureKind::Obstacle ||
                          feature->kind == minidayz::WorldFeatureKind::Water;
            if (feature->kind == minidayz::WorldFeatureKind::Building) {
                const auto* layer = minidayz::stage1::find_layer(
                    map_index, feature->source_layer);
                // Construct's building collision is attached to the actual
                // Buildings layer, not its decorative shadows/base duplicates.
                blocks = layer && std::strcmp(layer->name, "Buildings") == 0;
            }
            if (!blocks) continue;
            const float w = feature->width > 0.0f ? feature->width : 12.0f;
            const float h = feature->height > 0.0f ? feature->height : 12.0f;
            if (x >= feature->position.x && x <= feature->position.x + w &&
                y >= feature->position.y && y <= feature->position.y + h)
                return true;
        }
        return false;
    }

    minidayz::WorldSystem world_{};
    minidayz::NpcSystem npcs_{};
    std::uint32_t world_seed_{};
    float save_elapsed_{};
    bool world_ready_{};
    bool npc_ready_{};
    bool menu_active_{true};
    bool has_saved_game_{};
    minidayz::PlayerSystem player_;
    minidayz::items::InventorySystem inventory_{};
    minidayz::items::EquipmentSystem equipment_{};
    minidayz::items::GroundLootSystem ground_loot_{};
    minidayz::PlayerRules preview_rules_{};
    int selected_inventory_slot_{-1};
    int drag_source_slot_{-1};
    int drag_target_slot_{-1};
    int drag_target_equipment_slot_{-1};
    std::size_t inventory_page_{};
    bool touch_down_{};
};

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    minidayz::Engine engine;
    if (!engine.initialize()) {
        std::printf("Mini DAYZ+: engine initialization failed.\\n");
        return 1;
    }

    SavedGame restored{};
    const bool has_saved_game = load_saved_game(restored);
    WorldPreviewScene scene(has_saved_game ? &restored : nullptr);
    if (!engine.set_scene(&scene)) {
        std::printf("Mini DAYZ+: startup scene attachment failed.\\n");
        engine.shutdown();
        return 1;
    }

    engine.run();
    engine.shutdown();
    return 0;
}
