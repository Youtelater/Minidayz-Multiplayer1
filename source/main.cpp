#include "minidayz/engine.hpp"
#include "minidayz/player.hpp"
#include "minidayz/world.hpp"
#include "minidayz/npc_system.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/inventory.hpp"
#include "minidayz/equipment.hpp"
#include "minidayz/item_use.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/crafting.hpp"
#include "minidayz/tent_storage.hpp"
#include "minidayz/save_game.hpp"
#include "minidayz/perks.hpp"
#include "minidayz/stage2_audio.hpp"
#include "minidayz/item_description.hpp"
#include "minidayz/weather_source.hpp"
#include "minidayz/generated/item_catalog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {
using minidayz::persistence::SavedGame;
using minidayz::persistence::load_saved_game;
using minidayz::persistence::write_saved_game;

// Construct 2 creates a fresh random world on a new game; it does not use
// SEED_RUN as a numeric world seed. Use the 3DS clock and hardware tick as
// fresh entropy for this native session. This is a per-run seed, not a claim
// that the original browser Math.random() sequence is reproducible.
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
        if (restored && !inventory_.restore_with_satchel(restored->inventory, restored->satchel))
            inventory_.clear();
        if (restored && !equipment_.restore(restored->equipment))
            equipment_.clear();
        if (restored && !ground_loot_.restore(restored->ground_loot))
            ground_loot_.clear();
        if (restored && !tents_.restore(restored->tents))
            tents_.clear();
        if (restored && !item_use_runtime_.restore(restored->pending_item_use))
            item_use_runtime_.clear();
        if (restored && !perks_.restore(restored->perks)) perks_.clear();
        // Preview-only movement tuning. Source-equivalent values still need
        // to be supplied by the gameplay rules port.
        // Temporary preview movement tuning only. Original source movement
        // values are not guessed; they will replace these when the movement
        // event/ACE trace is complete.
        preview_rules_ = minidayz::source_survival_rules(
            minidayz::SurvivalDifficulty::regular);
        preview_rules_.walk_speed = 72.0f;
        preview_rules_.run_speed = 108.0f;
        preview_rules_.collision_half_width = 5.0f;
        preview_rules_.collision_half_height = 7.0f;
        preview_rules_.min_temperature = -10.0f;
        preview_rules_.max_temperature = 10.0f;
        world_ready_ = world_.generate_from_source(world_seed_, "Map", 0);
        if (world_ready_) npc_ready_ = npcs_.initialize(world_);
        // Continuing a save preserves its exact persisted temperature;
        // a fresh run initializes the original daytime exposure once.
        source_temperature_initialized_ = restored != nullptr;
        last_source_warmth_ = equipment_.temperature_offset();
        last_source_snowborn_level_ = perks_.level(minidayz::items::SourcePerk::snowborn);
    }

    void on_enter() noexcept override {
        // Audio is optional. RomFS is already mounted by Engine::initialize;
        // DSP failure must never prevent inventory or crafting gameplay.
        (void)stage2_audio_.initialize();
    }

    void on_exit() noexcept override {
        if (world_ready_ && !menu_active_)
            (void)write_saved_game(world_seed_, player_.state(), inventory_, equipment_, ground_loot_, tents_, &item_use_runtime_, &perks_);
        stage2_audio_.shutdown();
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
            const auto selected_index = static_cast<std::size_t>(selected_inventory_slot_);
            const auto* item = inventory_.slot(selected_index);
            const bool deploy_tent = item && !item->empty() &&
                item->item_id == minidayz::items::kOriginalCivilianTentItemId &&
                (input.pressed & KEY_X) != 0;
            if (deploy_tent) {
                (void)tents_.deploy_from_inventory_checked(
                    inventory_, selected_index, player_.state().x, player_.state().y,
                    &WorldPreviewScene::blocked, this);
            } else {
                (void)minidayz::items::use_item_from_inventory(
                    inventory_, selected_index, player_, preview_rules_,
                    (input.pressed & KEY_X) != 0, perks_.item_use_flags(), &item_use_runtime_);
            }
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

        // Cycle inventory -> crafting -> tent storage -> perks -> inventory.
        // Every Stage 2 panel stays on the lower 3DS touchscreen.
        if (touch_pressed && input.touch.py >= 223 &&
            input.touch.px >= 110 && input.touch.px < 210) {
            if (perks_open_) {
                perks_open_ = false;
            } else if (storage_open_) {
                storage_open_ = false;
                perks_open_ = true;
            } else if (crafting_open_) {
                crafting_open_ = false;
                storage_open_ = true;
            } else {
                crafting_open_ = true;
                details_open_ = false;
            }
            selected_inventory_slot_ = -1;
            drag_source_slot_ = -1;
            drag_target_slot_ = -1;
            drag_target_equipment_slot_ = -1;
        }
        if (crafting_open_ && touch_pressed && input.touch.py >= 223) {
            const std::size_t pages =
                (minidayz::items::kSourceCraftRecipeCount + kCraftPageSize - 1) /
                kCraftPageSize;
            if (input.touch.px < 110 && crafting_page_ > 0)
                --crafting_page_;
            else if (input.touch.px >= 210 && crafting_page_ + 1 < pages)
                ++crafting_page_;
        }
        if (crafting_open_ && touch_pressed &&
            input.touch.py >= 99 && input.touch.py < 219) {
            const std::size_t recipe = crafting_page_ * kCraftPageSize +
                static_cast<std::size_t>((input.touch.py - 99) / 40);
            if (minidayz::items::source_craft_recipe(recipe)) {
                const bool crafted = minidayz::items::craft_from_inventory(
                    inventory_, ground_loot_, player_.state().x, player_.state().y, recipe);
                crafting_feedback_ = crafted ? CraftFeedback::success : CraftFeedback::missing_or_full;
                crafting_feedback_elapsed_ = 0.0f;
                if (crafted) stage2_audio_.play_crafting();
            }
        }

        if (perks_open_ && touch_pressed && input.touch.py >= 223) {
            constexpr std::size_t page_size = 3;
            constexpr std::size_t page_count =
                (minidayz::items::kOriginalPerkCount + page_size - 1) /
                page_size;
            if (input.touch.px < 110 && perk_page_ > 0)
                --perk_page_;
            else if (input.touch.px >= 210 && perk_page_ + 1 < page_count)
                ++perk_page_;
        }
        if (perks_open_ && touch_pressed &&
            input.touch.px >= 16 && input.touch.px < 304 &&
            input.touch.py >= 110 && input.touch.py < 203) {
            constexpr std::size_t page_size = 3;
            const auto row = static_cast<std::size_t>(
                (input.touch.py - 110) / 31);
            const auto index = perk_page_ * page_size + row;
            if (index < minidayz::items::kOriginalPerkCount)
                (void)perks_.buy(minidayz::items::kOriginalPerkMenuOrder[index]);
        }

        if (storage_open_ && touch_pressed) {
            const auto tent_index = tents_.nearest(
                player_.state().x, player_.state().y, 36.0f);
            if (input.touch.py >= 84 && input.touch.py < 99) {
                if (input.touch.px < 160) {
                    const std::size_t pages =
                        (inventory_.capacity() + kInventoryPageSize - 1) / kInventoryPageSize;
                    inventory_page_ = (inventory_page_ + 1) % pages;
                } else {
                    const std::size_t pages =
                        (minidayz::items::kNativeTentStorageSlots +
                         kInventoryPageSize - 1) / kInventoryPageSize;
                    storage_page_ = (storage_page_ + 1) % pages;
                }
            } else if (tent_index < tents_.tent_count() &&
                       input.touch.py >= 99 && input.touch.py < 219) {
                const int col = input.touch.px < 160
                    ? (input.touch.px - 8) / 30 : (input.touch.px - 166) / 30;
                const int row = (input.touch.py - 99) / 40;
                if (col >= 0 && col < 5 && row >= 0 && row < 3) {
                    const std::size_t cell = static_cast<std::size_t>(row * 5 + col);
                    if (input.touch.px >= 8 && input.touch.px < 158)
                        (void)tents_.store_from_inventory(
                            inventory_, inventory_page_ * kInventoryPageSize + cell,
                            tent_index);
                    else if (input.touch.px >= 166 && input.touch.px < 316)
                        (void)tents_.take_to_inventory(
                            inventory_, tent_index,
                            storage_page_ * kInventoryPageSize + cell);
                }
            }
        }
        // Source-backed item operations are accessible without physical
        // buttons. This narrow action strip stays on the lower touchscreen.
        if (!crafting_open_ && !storage_open_ && !perks_open_ && touch_pressed &&
            input.touch.py >= 25 && input.touch.py < 38) {
            details_open_ = !details_open_;
        }
        if (!crafting_open_ && !storage_open_ && !perks_open_ && touch_pressed &&
            input.touch.py >= 81 && input.touch.py < 92) {
            const int action = input.touch.px / 64;
            if (action == 3) {
                (void)ground_loot_.pickup_nearest(
                    inventory_, player_.state().x, player_.state().y, 36.0f);
            } else if (selected_inventory_slot_ >= 0) {
                const auto selected = static_cast<std::size_t>(selected_inventory_slot_);
                const auto* item = inventory_.slot(selected);
                if (item && !item->empty()) {
                    if (action == 0 || action == 1) {
                        if (item->item_id == minidayz::items::kOriginalCivilianTentItemId &&
                            action == 1)
                            (void)tents_.deploy_from_inventory_checked(
                    inventory_, selected, player_.state().x, player_.state().y,
                    &WorldPreviewScene::blocked, this);
                        else
                            (void)minidayz::items::use_item_from_inventory(
                                inventory_, selected, player_, preview_rules_,
                                action == 1, perks_.item_use_flags(), &item_use_runtime_);
                    } else if (action == 2) {
                        (void)ground_loot_.drop_from_inventory(
                            inventory_, selected, 0, player_.state().x, player_.state().y);
                    } else if (action == 4) {
                        // Half-stack split on the 3DS lower touchscreen.
                        for (std::size_t i = 0; i < inventory_.capacity(); ++i) {
                            const auto* target = inventory_.slot(i);
                            if (target && target->empty() &&
                                inventory_.slot_accepts(i, item->item_id) &&
                                inventory_.split_half(selected, i)) {
                                selected_inventory_slot_ = static_cast<int>(i);
                                break;
                            }
                        }
                    }
                    item = inventory_.slot(selected);
                    if (!item || item->empty()) selected_inventory_slot_ = -1;
                }
            }
        }
        if (!crafting_open_ && !storage_open_ && !perks_open_ && !details_open_ && touch_pressed) {
            tap_was_selected_ = false;
            drag_source_slot_ = -1;
            drag_target_slot_ = -1;
            drag_target_equipment_slot_ = -1;
            const int equipment_target = equipment_slot_at(input.touch.px, input.touch.py);
            if (equipment_target >= 0) {
                if (equipment_target == 5) {
                    const auto backpack = inventory_.equipped_backpack();
                    const std::size_t destination =
                        first_available_base_inventory_slot();
                    if (backpack != 0 && destination < minidayz::items::kInventorySlotCount)
                        (void)inventory_.unequip_backpack_to_slot(destination);
                    selected_inventory_slot_ = -1;
                } else if (equipment_target == 6) {
                    const auto satchel = inventory_.equipped_satchel();
                    const auto destination = first_available_base_inventory_slot();
                    if (satchel != 0 && destination < minidayz::items::kInventorySlotCount)
                        (void)inventory_.unequip_satchel_to_slot(destination);
                    selected_inventory_slot_ = -1;
                } else {
                    const auto slot = static_cast<minidayz::items::EquipmentSlot>(equipment_target);
                    const auto equipped = equipment_.equipped(slot);
                    if (equipped != 0) {
                        const std::size_t destination = first_available_inventory_slot(equipped, equipment_.snapshot().conditions[static_cast<std::size_t>(slot)]);
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
                if (input.touch.px < 110 && inventory_page_ > 0)
                    --inventory_page_;
                else if (input.touch.px >= 210 &&
                         inventory_page_ + 1 < page_count)
                    ++inventory_page_;
                selected_inventory_slot_ = -1;
            } else {
                const int slot = inventory_slot_at(input.touch.px,
                                                   input.touch.py,
                                                   inventory_page_);
                if (slot >= 0) {
                    const auto* item = inventory_.slot(static_cast<std::size_t>(slot));
                    tap_was_selected_ = item && !item->empty() &&
                        selected_inventory_slot_ == slot;
                    selected_inventory_slot_ = item && !item->empty() ? slot : -1;
                    if (selected_inventory_slot_ >= 0) {
                        drag_source_slot_ = slot;
                        drag_target_slot_ = slot;
                    }
                }
            }
        }
        if (!crafting_open_ && !storage_open_ && !perks_open_ && !details_open_ && input.touching && drag_source_slot_ >= 0) {
            const int equipment_target = equipment_slot_at(input.touch.px, input.touch.py);
            if (equipment_target >= 0 && equipment_target < 7) {
                drag_target_equipment_slot_ = equipment_target;
                drag_target_slot_ = -1;
            } else {
                drag_target_equipment_slot_ = -1;
                const int target = inventory_slot_at(input.touch.px, input.touch.py,
                                                    inventory_page_);
                if (target >= 0) drag_target_slot_ = target;
            }
        }
        if (!crafting_open_ && !storage_open_ && !perks_open_ && !details_open_ && touch_released) {
            if (drag_source_slot_ >= 0 && drag_target_equipment_slot_ == 5) {
                const auto source_slot = static_cast<std::size_t>(drag_source_slot_);
                // Dragging an original repair material onto the backpack slot
                // invokes the source-defined repair event; otherwise it equips.
                if (inventory_.repair_equipped_backpack_from_inventory_slot(source_slot) ||
                    inventory_.equip_backpack_from_slot(source_slot))
                    selected_inventory_slot_ = -1;
            } else if (drag_source_slot_ >= 0 && drag_target_equipment_slot_ == 6) {
                if (inventory_.equip_satchel_from_slot(
                        static_cast<std::size_t>(drag_source_slot_)))
                    selected_inventory_slot_ = -1;
            } else if (drag_source_slot_ >= 0 && drag_target_equipment_slot_ >= 0) {
                const auto source_slot = static_cast<std::size_t>(drag_source_slot_);
                const auto target = static_cast<minidayz::items::EquipmentSlot>(
                    drag_target_equipment_slot_);
                bool changed = equipment_.equip_from_inventory(
                    inventory_, source_slot, target);
                if (!changed) {
                    minidayz::items::SourceRepairTarget repair_target{};
                    bool repairable = true;
                    switch (target) {
                    case minidayz::items::EquipmentSlot::head: {
                        const auto* material = inventory_.slot(source_slot);
                        std::uint8_t gain = 0;
                        repair_target =
                            material && minidayz::items::source_item_repair_gain(
                                material->item_id,
                                minidayz::items::SourceRepairTarget::helmet, gain)
                            ? minidayz::items::SourceRepairTarget::helmet
                            : minidayz::items::SourceRepairTarget::helmet_lite;
                        break;
                    }
                    case minidayz::items::EquipmentSlot::outerwear:
                        repair_target = minidayz::items::SourceRepairTarget::outerwear;
                        break;
                    case minidayz::items::EquipmentSlot::vest:
                        repair_target = minidayz::items::SourceRepairTarget::vest;
                        break;
                    case minidayz::items::EquipmentSlot::pants:
                        repair_target = minidayz::items::SourceRepairTarget::pants;
                        break;
                    default:
                        repairable = false;
                        break;
                    }
                    if (repairable)
                        changed = equipment_.repair_from_inventory(
                            inventory_, source_slot, repair_target);
                }
                if (changed) selected_inventory_slot_ = -1;
            } else if (drag_source_slot_ >= 0 && drag_target_slot_ >= 0 &&
                       drag_target_slot_ != drag_source_slot_) {
                if (inventory_.move(static_cast<std::size_t>(drag_source_slot_),
                                    static_cast<std::size_t>(drag_target_slot_)))
                    selected_inventory_slot_ = drag_target_slot_;
            } else if (drag_source_slot_ >= 0 &&
                       drag_target_slot_ == drag_source_slot_ &&
                       tap_was_selected_) {
                // Touch activates the already-selected original primary Use
                // effect, or deploys the original civilian tent.
                const auto source = static_cast<std::size_t>(drag_source_slot_);
                const auto* item = inventory_.slot(source);
                if (item && !item->empty()) {
                    if (item->item_id == minidayz::items::kOriginalCivilianTentItemId)
                        (void)tents_.deploy_from_inventory_checked(
                    inventory_, source, player_.state().x, player_.state().y,
                    &WorldPreviewScene::blocked, this);
                    else
                        (void)minidayz::items::use_item_from_inventory(
                            inventory_, source, player_, preview_rules_,
                            false, perks_.item_use_flags(), &item_use_runtime_);
                    item = inventory_.slot(source);
                    if (!item || item->empty()) selected_inventory_slot_ = -1;
                }
            }
            tap_was_selected_ = false;
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
        if (crafting_feedback_ != CraftFeedback::none) {
            crafting_feedback_elapsed_ += std::max(0.0f, dt);
            if (crafting_feedback_elapsed_ >= 2.5f) {
                crafting_feedback_ = CraftFeedback::none;
                crafting_feedback_elapsed_ = 0.0f;
            }
        }
        const float current_source_warmth = equipment_.temperature_offset();
        const auto snowborn_level =
            perks_.level(minidayz::items::SourcePerk::snowborn);
        if (!source_temperature_initialized_ ||
            current_source_warmth != last_source_warmth_ ||
            snowborn_level != last_source_snowborn_level_) {
            float source_temperature = 0.0f;
            if (minidayz::items::source_weather_temperature_with_snowborn(
                    minidayz::SurvivalDifficulty::regular,
                    minidayz::items::SourceColdPhase::day,
                    current_source_warmth,
                    minidayz::items::SourceRainColdBranch::full,
                    snowborn_level, source_temperature))
                player_.set_source_temperature(source_temperature);
            last_source_warmth_ = current_source_warmth;
            last_source_snowborn_level_ = snowborn_level;
            source_temperature_initialized_ = true;
        }
        player_.update_movement(movement, dt, preview_rules_,
                                &WorldPreviewScene::blocked, this);
        player_.update_survival(dt, preview_rules_);
        item_use_runtime_.update(dt, player_,
            &WorldPreviewScene::award_source_experience, &perks_);
        if (npc_ready_) npcs_.update(dt, player_.state().x, player_.state().y,
                                     &WorldPreviewScene::blocked, this);
        save_elapsed_ += dt;
        if (save_elapsed_ >= 2.0f) {
            save_elapsed_ = 0.0f;
            (void)write_saved_game(world_seed_, player_.state(), inventory_, equipment_, ground_loot_, tents_, &item_use_runtime_, &perks_);
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
        // Show the exact source-localized item name above the touch slots.
        // The source is the original root l_eng_items.xml, never a guessed ID
        // or a name from the excluded newer Minidayz folder.
        if (selected_inventory_slot_ >= 0) {
            const auto* selected = inventory_.slot(
                static_cast<std::size_t>(selected_inventory_slot_));
            if (selected && !selected->empty()) {
                if (const char* name = minidayz::items::source_item_name(
                        selected->item_id))
                    renderer.draw_text(name, 10.0f, 25.0f, 0.19f);
            }
        }
        // Bleeding/infection and temperature remain visible while managing
        // items. These are the actual player state values, not guessed effects.
        char survival_labels[96]{};
        std::snprintf(survival_labels, sizeof(survival_labels),
                      "T %.1f   BLEED %.1f   INFECT %.1f",
                      static_cast<double>(stats.temperature),
                      static_cast<double>(stats.bleeding),
                      static_cast<double>(stats.infection));
        renderer.draw_text(survival_labels, 163.0f, 25.0f, 0.14f);
        // Original Regular difficulty sets Day_cold=7. This displays the
        // exact source clothing-warmth expression without fabricating
        // weather cadence or cold-damage rates.
        const auto& weather = minidayz::items::source_weather_cold(
            minidayz::SurvivalDifficulty::regular);
        char cold_status[64]{};
        std::snprintf(cold_status, sizeof(cold_status),
            "DAY COLD %.0f - WARMTH %.0f = %.0f",
            static_cast<double>(weather.day),
            static_cast<double>(equipment_.temperature_offset()),
            static_cast<double>(equipment_.day_cooling(weather.day)));
        renderer.draw_text(cold_status, 10.0f, 33.0f, 0.15f);
        if (item_use_runtime_.busy()) {
            char treatment[48]{};
            const auto* rule = minidayz::items::ItemUseRuntime::medical_rule(
                item_use_runtime_.snapshot().pending_medical_item_id);
            const auto pending_item_id = item_use_runtime_.snapshot().pending_medical_item_id;
            std::snprintf(treatment, sizeof(treatment), "%s %.1fs",
                pending_item_id == minidayz::items::ItemUseRuntime::kOriginalNewspaperItemId
                    ? "READING +50XP" :
                pending_item_id == minidayz::items::ItemUseRuntime::kOriginalScoutHandbookItemId
                    ? "READING +200XP" :
                    rule && rule->action == minidayz::items::SourceMedicalAction::stop_bleeding
                        ? "BANDAGING" : "TREATMENT",
                static_cast<double>(item_use_runtime_.remaining()));
            renderer.draw_text(treatment, 170.0f, 33.0f, 0.18f);
        }
        // Original equipment categories: helmet, outerwear, vest, pants,
        // held weapon, plus the separate backpack slot.
        static constexpr const char* equipment_labels[] = {"HELM","COAT","VEST","PANTS","HAND","PACK","SATCH"};
        for (std::size_t i = 0; i < 7; ++i) {
            const float x = 5.0f + static_cast<float>(i) * 45.0f;
            renderer.draw_solid_rect(x, 38.0f, 43.0f, 43.0f, C2D_Color32(8, 11, 10, 255));
            renderer.draw_solid_rect(x + 2.0f, 40.0f, 39.0f, 39.0f, C2D_Color32(47, 54, 48, 255));
            const auto item_id = i == 5 ? inventory_.equipped_backpack()
                : i == 6 ? inventory_.equipped_satchel()
                : equipment_.equipped(static_cast<minidayz::items::EquipmentSlot>(i));
            const auto condition = i == 5 ? inventory_.equipped_backpack_condition()
                : i == 6 ? inventory_.equipped_satchel_condition()
                : equipment_.snapshot().conditions[i];
            if (item_id != 0) {
                (void)renderer.draw_item_icon(item_id, x + 21.5f, 55.0f, 30.0f, 25.0f);
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
        if (!storage_open_ && !crafting_open_) {
            static constexpr const char* actions[] = {"USE", "ALT", "DROP", "PICK", "SPLIT"};
            for (int i = 0; i < 5; ++i) {
                const float x = static_cast<float>(i * 64);
                renderer.draw_solid_rect(x + 1.0f, 81.0f, 62.0f, 11.0f,
                                         C2D_Color32(65, 78, 66, 255));
                renderer.draw_text(actions[i], x + 13.0f, 83.0f, 0.21f);
            }
        }
        if (perks_open_) {
            // Source Perks and stats menu has 16 two-level perk variables.
            // Do not display unported bonuses as if they are active.
            char status[96]{};
            std::snprintf(status, sizeof(status), "XP %lu NEXT %lu",
                static_cast<unsigned long>(perks_.snapshot().experience_points),
                static_cast<unsigned long>(perks_.snapshot().perk_cost));
            renderer.draw_solid_rect(8.0f, 89.0f, 304.0f, 133.0f,
                                     C2D_Color32(22, 31, 25, 255));
            renderer.draw_text(status, 20.0f, 95.0f, 0.28f);
            constexpr std::size_t perk_page_size = 3;
            for (std::size_t row = 0; row < perk_page_size; ++row) {
                const auto index = perk_page_ * perk_page_size + row;
                if (index >= minidayz::items::kOriginalPerkCount) break;
                const auto perk = minidayz::items::kOriginalPerkMenuOrder[index];
                const float y = 110.0f + static_cast<float>(row) * 31.0f;
                renderer.draw_solid_rect(16.0f, y, 288.0f, 28.0f,
                    perks_.can_buy(perk) ? C2D_Color32(65, 95, 64, 255)
                                         : C2D_Color32(49, 56, 49, 255));
                char label[80]{};
                std::snprintf(label, sizeof(label), "%s  %u / 2",
                    minidayz::items::kOriginalPerkMenuLabels[index],
                    static_cast<unsigned>(perks_.level(perk)));
                renderer.draw_text(label, 22.0f, y + 7.0f, 0.32f);
                renderer.draw_text(perks_.level(perk) >= 2 ? "MAX" :
                    perks_.can_buy(perk) ? "BUY" : "LOCKED",
                    245.0f, y + 8.0f, 0.24f);
            }
            char page_label[56]{};
            std::snprintf(page_label, sizeof(page_label),
                          "ORIGINAL PERKS  PAGE %u / %u",
                          static_cast<unsigned>(perk_page_ + 1), 6u);
            renderer.draw_text(page_label, 17.0f, 207.0f, 0.23f);
            renderer.draw_solid_rect(10.0f, 225.0f, 300.0f, 13.0f,
                                     C2D_Color32(54, 68, 55, 255));
            renderer.draw_text("< PREV", 14.0f, 226.0f, 0.25f);
            renderer.draw_text("BACK TO BAG", 122.0f, 226.0f, 0.25f);
            renderer.draw_text("NEXT >", 262.0f, 226.0f, 0.25f);
            return;
        }
        if (storage_open_) {
            // The tent item and permanent storage behavior are verified from
            // root original XML. Native list bounds are resource safeguards,
            // not invented gameplay capacities.
            renderer.draw_solid_rect(6.0f, 85.0f, 308.0f, 137.0f,
                                     C2D_Color32(22, 31, 25, 255));
            const auto tent_index = tents_.nearest(
                player_.state().x, player_.state().y, 36.0f);
            const auto* tent = tents_.tent(tent_index);
            if (!tent) {
                renderer.draw_text("NO DEPLOYED TENT NEARBY", 22.0f, 130.0f, 0.36f);
                renderer.draw_text("SELECT CIVILIAN TENT IN BAG, PRESS X", 14.0f, 153.0f, 0.25f);
            } else {
                char labels[64]{};
                std::snprintf(labels, sizeof(labels), "BAG %u    TAP TO PAGE    TENT %u",
                    static_cast<unsigned>(inventory_page_ + 1),
                    static_cast<unsigned>(storage_page_ + 1));
                renderer.draw_text(labels, 12.0f, 87.0f, 0.28f);
                for (int row = 0; row < 3; ++row) {
                    for (int col = 0; col < 5; ++col) {
                        const std::size_t offset = static_cast<std::size_t>(row * 5 + col);
                        const auto* carried = inventory_.slot(
                            inventory_page_ * kInventoryPageSize + offset);
                        const auto tent_slot = storage_page_ * kInventoryPageSize + offset;
                        const auto* stored = tent_slot < tent->items.size()
                            ? &tent->items[tent_slot] : nullptr;
                        for (int side = 0; side < 2; ++side) {
                            const auto* item = side == 0 ? carried : stored;
                            const float x = static_cast<float>(
                                (side == 0 ? 8 : 166) + col * 30);
                            const float y = static_cast<float>(99 + row * 40);
                            renderer.draw_solid_rect(x, y, 28.0f, 37.0f,
                                                     C2D_Color32(48, 57, 48, 255));
                            if (!item || item->empty()) continue;
                            (void)renderer.draw_item_icon(item->item_id,
                                x + 14.0f, y + 15.0f, 24.0f, 24.0f);
                            char amount[12]{};
                            std::snprintf(amount, sizeof(amount), "%u",
                                          static_cast<unsigned>(item->count));
                            renderer.draw_text(amount, x + 2.0f, y + 29.0f, 0.19f);
                        }
                    }
                }
            }
            renderer.draw_solid_rect(10.0f, 225.0f, 300.0f, 13.0f,
                                     C2D_Color32(54, 68, 55, 255));
            renderer.draw_text("TAP TO STORE / TAKE", 14.0f, 226.0f, 0.19f);
            renderer.draw_text("PERKS", 132.0f, 226.0f, 0.29f);
            return;
        }
        if (crafting_open_) {
            // Source-verified crafting recipes and exact item IDs. Every
            // ingredient line shows owned/required counts; the original
            // output destination (ground or inventory) is visible before use.
            renderer.draw_solid_rect(8.0f, 89.0f, 304.0f, 133.0f,
                                     C2D_Color32(22, 31, 25, 255));
            const std::size_t page_count =
                (minidayz::items::kSourceCraftRecipeCount + kCraftPageSize - 1) /
                kCraftPageSize;
            char page_header[60]{};
            std::snprintf(page_header, sizeof(page_header),
                          "CRAFTING %u/%u     TAP A RECIPE",
                          static_cast<unsigned>(crafting_page_ + 1),
                          static_cast<unsigned>(page_count));
            renderer.draw_text(page_header, 13.0f, 90.0f, 0.19f);
            for (std::size_t row = 0; row < kCraftPageSize; ++row) {
                const std::size_t i = crafting_page_ * kCraftPageSize + row;
                const auto* recipe = minidayz::items::source_craft_recipe(i);
                if (!recipe) continue;
                const float y = 99.0f + static_cast<float>(row) * 40.0f;
                const bool ready = minidayz::items::can_craft_from_inventory(
                    inventory_, ground_loot_, player_.state().x, player_.state().y, i);
                renderer.draw_solid_rect(12.0f, y, 296.0f, 36.0f,
                    ready ? C2D_Color32(65, 95, 64, 255)
                          : C2D_Color32(49, 56, 49, 255));
                (void)renderer.draw_item_icon(recipe->result_id,
                    27.0f, y + 16.0f, 24.0f, 24.0f);
                // The result name is original localization where available.
                const char* result_name = minidayz::items::source_item_name(
                    recipe->result_id);
                renderer.draw_text(result_name ? result_name : recipe->result_name,
                                   43.0f, y + 2.0f, 0.29f);
                renderer.draw_text(ready ? "READY" : "NEED",
                                   263.0f, y + 2.0f, 0.19f);
                char ingredients[145]{};
                std::size_t at = 0;
                for (std::size_t part = 0; part < recipe->ingredients.size(); ++part) {
                    const auto ingredient = recipe->ingredients[part];
                    if (ingredient.count == 0) continue;
                    std::uint32_t owned = inventory_.count(ingredient.item_id);
                    if (recipe->selected_stack_only) {
                        // item_activate2 verifies a single Rags stack, not
                        // the cumulative total across the backpack.
                        owned = 0;
                        for (std::size_t slot_index = 0;
                             slot_index < inventory_.capacity(); ++slot_index) {
                            const auto* stack = inventory_.slot(slot_index);
                            if (stack && stack->item_id == ingredient.item_id)
                                owned = std::max<std::uint32_t>(owned, stack->count);
                        }
                    }
                    const char* name = minidayz::items::source_item_name(ingredient.item_id);
                    const int written = std::snprintf(ingredients + at,
                        sizeof(ingredients) - at, "%s%s %u/%u%s",
                        at ? " + " : "", name ? name : "ITEM",
                        static_cast<unsigned>(owned),
                        static_cast<unsigned>(ingredient.count),
                        part == 1 && recipe->preserve_second_ingredient ? " (tool)" : "");
                    if (written <= 0) break;
                    at += std::min(static_cast<std::size_t>(written),
                                   sizeof(ingredients) - at - 1);
                }
                renderer.draw_text(ingredients, 43.0f, y + 19.0f, 0.19f);
                renderer.draw_text(recipe->result_in_inventory ? "BAG" : "GROUND",
                                   265.0f, y + 26.0f, 0.16f);
            }
            if (crafting_feedback_ != CraftFeedback::none) {
                const bool success = crafting_feedback_ == CraftFeedback::success;
                renderer.draw_solid_rect(10.0f, 209.0f, 300.0f, 13.0f,
                    success ? C2D_Color32(46, 105, 52, 255)
                            : C2D_Color32(115, 66, 46, 255));
                renderer.draw_text(success ? "CRAFTED - CHECK BAG OR GROUND"
                                         : "NEED MATERIALS OR OUTPUT SPACE",
                                   24.0f, 211.0f, 0.22f);
            }
            renderer.draw_solid_rect(10.0f, 225.0f, 300.0f, 13.0f,
                                     C2D_Color32(54, 68, 55, 255));
            renderer.draw_text("< PREV", 14.0f, 226.0f, 0.25f);
            renderer.draw_text("TENTS", 132.0f, 226.0f, 0.29f);
            renderer.draw_text("NEXT >", 262.0f, 226.0f, 0.25f);
            return;
        }
        if (details_open_) {
            renderer.draw_solid_rect(8.0f, 94.0f, 304.0f, 126.0f,
                                     C2D_Color32(24, 37, 28, 255));
            const auto* selected = selected_inventory_slot_ >= 0
                ? inventory_.slot(static_cast<std::size_t>(selected_inventory_slot_))
                : nullptr;
            if (selected && !selected->empty()) {
                const char* description =
                    minidayz::items::source_item_description(selected->item_id);
                char heading[80]{};
                std::snprintf(heading, sizeof(heading), "ITEM %u  x%u  CONDITION %u%%",
                    static_cast<unsigned>(selected->item_id),
                    static_cast<unsigned>(selected->count),
                    static_cast<unsigned>(selected->condition));
                renderer.draw_text(heading, 16.0f, 101.0f, 0.27f);
                if (description && *description) {
                    // Simple bounded word-wrapping for the original XML text.
                    // Do not substitute guessed descriptions for missing rows.
                    const char* pos = description;
                    for (int line = 0; line < 6 && *pos; ++line) {
                        char buffer[58]{};
                        std::size_t length = 0;
                        const char* next = pos;
                        const char* split = nullptr;
                        while (*next && *next != '\n' && length < 50) {
                            if (*next == ' ') split = next;
                            ++next; ++length;
                        }
                        if (*next && *next != '\n' && length == 50 && split) {
                            length = static_cast<std::size_t>(split - pos);
                            next = split;
                        }
                        std::memcpy(buffer, pos, length);
                        buffer[length] = '\0';
                        renderer.draw_text(buffer, 16.0f,
                            125.0f + static_cast<float>(line) * 14.0f, 0.24f);
                        pos = next;
                        while (*pos == ' ' || *pos == '\n') ++pos;
                    }
                } else {
                    renderer.draw_text("NO ORIGINAL DESCRIPTION IN ROOT XML",
                                       16.0f, 134.0f, 0.23f);
                }
            } else {
                renderer.draw_text("SELECT AN ITEM, THEN OPEN DETAILS",
                                   16.0f, 130.0f, 0.26f);
            }
            renderer.draw_text("TAP ITEM NAME ABOVE TO CLOSE",
                               14.0f, 205.0f, 0.20f);
            renderer.draw_solid_rect(10.0f, 225.0f, 300.0f, 13.0f,
                                     C2D_Color32(54, 68, 55, 255));
            renderer.draw_text("ORIGINAL ITEM DESCRIPTION", 35.0f, 226.0f, 0.24f);
            renderer.draw_text("CRAFT", 132.0f, 226.0f, 0.29f);
            return;
        }
        // 5 x 3 backpack page, sized for the 320 x 240 touch screen.
        const int page_start = static_cast<int>(inventory_page_ * kInventoryPageSize);
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 5; ++col) {
                const int index = page_start + row * 5 + col;
                const float x = 10.0f + col * 60.0f;
                const float y = 92.0f + row * 43.0f;
                const bool selected = index == selected_inventory_slot_;
                const bool restricted = index >= 0 &&
                    inventory_.is_exclusive_satchel_slot(
                        static_cast<std::size_t>(index));
                renderer.draw_solid_rect(x, y, 56.0f, 39.0f,
                    selected ? C2D_Color32(225, 190, 105, 255)
                             : restricted ? C2D_Color32(105, 130, 126, 255)
                             : C2D_Color32(5, 8, 7, 255));
                renderer.draw_solid_rect(x + 2.0f, y + 2.0f, 52.0f, 35.0f,
                    selected ? C2D_Color32(76, 76, 54, 255)
                             : restricted ? C2D_Color32(33, 62, 57, 255)
                             : C2D_Color32(43, 49, 44, 255));
                if (restricted) {
                    const auto satchel_id = inventory_.equipped_satchel();
                    const char* label =
                        (satchel_id == 459 || satchel_id == 753)
                            ? "ARMY" : "MED";
                    renderer.draw_text(label, x + 4.0f, y + 3.0f, 0.17f);
                }
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
        renderer.draw_text("TOUCH ACTIONS ABOVE", 14.0f, 226.0f, 0.18f);
        renderer.draw_text("CRAFT", 132.0f, 226.0f, 0.30f);
        renderer.draw_text(page_label, 213.0f, 226.0f, 0.23f);
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

        for (std::size_t i = 0; i < tents_.tent_count(); ++i) {
            const auto* tent = tents_.tent(i);
            if (!tent || tent->x < camera_x - 24.0f ||
                tent->x > camera_x + view_w + 24.0f ||
                tent->y < camera_y - 24.0f ||
                tent->y > camera_y + view_h + 24.0f) continue;
            (void)renderer.draw_item_icon(minidayz::items::kOriginalCivilianTentItemId,
                                          tent->x, tent->y, 28.0f, 28.0f);
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
    static bool award_source_experience(void* context,
                                        std::uint32_t amount) noexcept {
        auto* perks = static_cast<minidayz::items::SourcePerkSystem*>(context);
        return perks && perks->award_experience(amount);
    }

    static constexpr std::size_t kInventoryPageSize = 15;
    static constexpr std::size_t kCraftPageSize = 3;

    static int equipment_slot_at(int x, int y) noexcept {
        if (y < 38 || y >= 81 || x < 5 || x >= 320) return -1;
        const int index = (x - 5) / 45;
        return index >= 0 && index < 7 ? index : -1;
    }

    std::size_t first_available_inventory_slot(std::uint16_t item_id,
                                               std::uint8_t condition) const noexcept {
        const auto limit = minidayz::items::source_max_stack(item_id);
        std::size_t first_empty = inventory_.capacity();
        for (std::size_t i = 0; i < inventory_.capacity(); ++i) {
            if (!inventory_.slot_accepts(i, item_id)) continue;
            const auto* item = inventory_.slot(i);
            if (!item) continue;
            if (item->empty()) {
                if (first_empty == inventory_.capacity()) first_empty = i;
            } else if (item->item_id == item_id &&
                       item->condition == condition && item->count < limit) {
                // Preserve free space by merging only matching-condition stacks.
                return i;
            }
        }
        return first_empty;
    }

    // The original backpack equipment slot is outside the 15 base slots.
    // Unequipping must return it to a base slot; an empty backpack-storage
    // slot cannot be used as the destination while shrinking the grid.
    std::size_t first_available_base_inventory_slot() const noexcept {
        // Backpack/satchel unequip requires an EMPTY surviving base cell.
        // A matching partial stack cannot receive a whole equipped bag.
        for (std::size_t i = 0; i < minidayz::items::kInventorySlotCount; ++i) {
            const auto* item = inventory_.slot(i);
            if (item && item->empty()) return i;
        }
        return minidayz::items::kInventorySlotCount;
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
        tents_.clear();
        item_use_runtime_.clear();
        perks_.clear();
        source_temperature_initialized_ = false;
        last_source_warmth_ = 0.0f;
        last_source_snowborn_level_ = 0;
        inventory_page_ = 0;
        storage_page_ = 0;
        crafting_page_ = 0;
        perk_page_ = 0;
        crafting_feedback_ = CraftFeedback::none;
        crafting_feedback_elapsed_ = 0.0f;
        storage_open_ = false;
        crafting_open_ = false;
        perks_open_ = false;
        details_open_ = false;
        tap_was_selected_ = false;
        selected_inventory_slot_ = -1;
        world_ready_ = world_.generate_from_source(world_seed_, "Map", 0);
        npc_ready_ = world_ready_ && npcs_.initialize(world_);
        save_elapsed_ = 0.0f;
        has_saved_game_ = false;
        // Never enter a blank gameplay scene when source world generation
        // fails. Keep the menu available so the player can retry instead.
        menu_active_ = !world_ready_;
        if (world_ready_) (void)write_saved_game(world_seed_, player_.state(), inventory_, equipment_, ground_loot_, tents_, &item_use_runtime_, &perks_);
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
    minidayz::items::TentStorageSystem tents_{};
    minidayz::items::ItemUseRuntime item_use_runtime_{};
    minidayz::items::SourcePerkSystem perks_{};
    minidayz::Stage2Audio stage2_audio_{};
    minidayz::PlayerRules preview_rules_{};
    bool source_temperature_initialized_{};
    float last_source_warmth_{};
    std::uint8_t last_source_snowborn_level_{};
    int selected_inventory_slot_{-1};
    int drag_source_slot_{-1};
    int drag_target_slot_{-1};
    int drag_target_equipment_slot_{-1};
    std::size_t inventory_page_{};
    std::size_t storage_page_{};
    std::size_t crafting_page_{};
    std::size_t perk_page_{};
    enum class CraftFeedback : std::uint8_t { none, success, missing_or_full };
    CraftFeedback crafting_feedback_{CraftFeedback::none};
    float crafting_feedback_elapsed_{};
    bool touch_down_{};
    bool tap_was_selected_{};
    bool crafting_open_{};
    bool storage_open_{};
    bool perks_open_{};
    bool details_open_{};
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
