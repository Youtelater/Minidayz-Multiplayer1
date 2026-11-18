#include "minidayz/engine.hpp"
#include "minidayz/boot_menu.hpp"
#include "minidayz/player.hpp"
#include "minidayz/world.hpp"
#include "minidayz/terrain_collision.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include "minidayz/terrain_source_visibility.hpp"
#include "minidayz/generated/sprite_catalog.hpp"
#include "minidayz/generated/player_skin_source.hpp"
#include "minidayz/player_equipment_visual.hpp"
#include "minidayz/npc_system.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/inventory.hpp"
#include "minidayz/equipment.hpp"
#include "minidayz/item_use.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/crafting.hpp"
#include "minidayz/tent_storage.hpp"
#include "minidayz/stash_storage.hpp"
#include "minidayz/save_game.hpp"
#include "minidayz/perks.hpp"
#include "minidayz/stage2_audio.hpp"
#include "minidayz/item_description.hpp"
#include "minidayz/weather_source.hpp"
#include "minidayz/source_day_night.hpp"
#include "minidayz/source_rain_timer.hpp"
#include "minidayz/source_loot_timer.hpp"
#include "minidayz/original_timed_loot.hpp"
#include "minidayz/generated/inventory_ui_art.hpp"
#include "minidayz/generated/item_catalog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <sys/stat.h>

namespace {
// Original Start events initialize rains_level=2 for the normal source map.
// Rain event 609331584746158 chooses blizzard.wav specifically for value 2.
constexpr std::uint8_t kSourceDefaultRainsLevel=2;
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
          world_level_(restored ? restored->world_level : 0),
          has_saved_game_(restored != nullptr),
          player_(restored ? restored->player : make_initial_state()) {
        player_.set_source_skin(
            minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
        if (restored && !inventory_.restore_with_satchel(restored->inventory, restored->satchel))
            inventory_.clear();
        if (restored && !equipment_.restore_with_fraction(
                restored->equipment, restored->equipment_fraction))
            equipment_.clear();
        if (restored && !ground_loot_.restore(restored->ground_loot))
            ground_loot_.clear();
        if (restored && !tents_.restore(restored->tents))
            tents_.clear();
        if (restored && !stashes_.restore(restored->stashes))
            stashes_.clear();
        if (restored && !day_night_.restore(restored->source_time))
            day_night_.reset();
        if (restored && !rain_timer_.restore(restored->source_rain))
            rain_timer_.reset();
        if (restored && !loot_timer_.restore(restored->source_loot))
            loot_timer_.reset();
        if (restored && restored->pending_craft.recipe_index != 0xffffu) {
            pending_craft_recipe_ = restored->pending_craft.recipe_index;
            pending_craft_seconds_ = restored->pending_craft.seconds_remaining;
            crafting_feedback_ = CraftFeedback::pending;
        }
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
        // The boot menu must appear immediately, before world generation.
        // Continue reconstructs the saved world only after the player selects it.
        world_ready_ = false;
        npc_ready_ = false;
        // Continuing a save preserves its exact persisted temperature;
        // a fresh run initializes the original daytime exposure once.
        // A save persists body warmth; weather exposure is recalculated
        // separately from the current world/clothing state on Continue.
        source_temperature_initialized_ = false;
        last_source_warmth_ = equipment_.temperature_offset();
        last_source_snowborn_level_ = perks_.level(minidayz::items::SourcePerk::snowborn);
    }

    bool write_scene_save() noexcept {
        minidayz::persistence::PendingCraftSnapshot craft{};
        if (pending_craft_recipe_ < minidayz::items::kSourceCraftRecipeCount &&
            pending_craft_seconds_ > 0.0f) {
            craft.recipe_index = static_cast<std::uint16_t>(pending_craft_recipe_);
            craft.seconds_remaining = pending_craft_seconds_;
        }
        return write_saved_game(world_seed_, player_.state(), inventory_,
            equipment_, ground_loot_, tents_, &item_use_runtime_, &perks_,
            &stashes_, &day_night_, &rain_timer_, &loot_timer_, &craft,
            world_level_);
    }

    void on_enter() noexcept override {
        // The source tilemap_water and tilemap_forest sprites carry Solid
        // behavior. Load the exact Map RLE occupancy once from the RomFS
        // so the player collides with visible tiles, not huge empty tilemap
        // bounding boxes. Failure leaves basic sprite collision enabled.
        (void)terrain_collision_.load(
            "romfs:/gfx/original_map_details_layout.bin");
        // Audio is optional. RomFS is already mounted by Engine::initialize;
        // DSP failure must never prevent inventory or crafting gameplay.
        (void)stage2_audio_.initialize();
        // World audio must not start over the boot menu. Its initial
        // environment loops are enabled after generation finishes.
    }

    void on_exit() noexcept override {
        if (world_ready_ && !menu_active_)
            (void)write_scene_save();
        stage2_audio_.shutdown();
    }

    void update(const minidayz::InputState& input, float dt) noexcept override {
        if (loading_world_) {
            if (!world_.advance_source_generation()) {
                loading_world_ = false;
                generation_failed_ = true;
                return;
            }
            if (world_.generation_complete()) {
                loading_world_ = false;
                world_ready_ = true;
                npc_ready_ = npcs_.initialize(world_);
                if (new_game_generation_ && !place_new_player_on_generated_map()) {
                    // Do not start a new run at a hard-coded, blocked map edge.
                    loading_world_ = false;
                    world_ready_ = false;
                    generation_failed_ = true;
                    return;
                }
                new_game_generation_ = false;
                travel_generation_ = false;
                menu_active_ = false;
                stage2_audio_.set_environment_ambience(
                    !day_night_.day_mode(),world_level_==4);
                if (rain_timer_.active()) stage2_audio_.start_rain_loop(kSourceDefaultRainsLevel==2);
                else stage2_audio_.stop_rain_loop();
                // Only overwrite the existing save after a new world succeeds.
                has_saved_game_ = write_scene_save();
            }
            return;
        }
        if (menu_active_) {
            // Original player skins have their own Construct sprite objects.
            // L/R or touching the bottom-screen skin selector cycles the
            // actual original texture, never a recolored default sprite.
            if ((input.pressed & KEY_L) != 0)
                selected_skin_ = (selected_skin_ +
                    minidayz::stage1::kOriginalPlayerSkinCount - 1) %
                    minidayz::stage1::kOriginalPlayerSkinCount;
            if ((input.pressed & KEY_R) != 0)
                selected_skin_ = (selected_skin_ + 1) %
                    minidayz::stage1::kOriginalPlayerSkinCount;
            // The game always boots here, even when a saved run exists.
            // A Continue tap/press starts loading only after selection.
            auto action = minidayz::menu::button_action(
                (input.pressed & KEY_A) != 0,
                (input.pressed & KEY_X) != 0, has_saved_game_);
            const bool menu_tap = input.touching && !touch_down_;
            touch_down_ = input.touching;
            if (menu_tap) {
                if (input.touch.py >= 184 && input.touch.py < 203) {
                    const auto total=minidayz::stage1::kOriginalPlayerSkinCount;
                    if(input.touch.px < 160)
                        selected_skin_=(selected_skin_+total-1)%total;
                    else
                        selected_skin_=(selected_skin_+1)%total;
                } else {
                    const auto tapped = minidayz::menu::touch_action(
                        input.touch.px, input.touch.py, has_saved_game_);
                    if (tapped != minidayz::menu::BootAction::none)
                        action = tapped;
                }
            }
            if (action == minidayz::menu::BootAction::new_game)
                start_new_game();
            else if (action == minidayz::menu::BootAction::continue_game)
                begin_world_generation();
            return;
        }
        // Once the original survival rules kill the player, stop gameplay
        // updates and present a 3DS touch/physical-button death screen.
        // Do not permit dead characters to use, craft or transfer items.
        if (!player_.state().alive) {
            if (!death_saved_) {
                death_saved_ = write_scene_save();
                has_saved_game_ = has_saved_game_ || death_saved_;
            }
            const bool touch_pressed = input.touching && !touch_down_;
            touch_down_ = input.touching;
            bool restart = (input.pressed & KEY_X) != 0;
            bool to_menu = (input.pressed & KEY_Y) != 0;
            if (touch_pressed && input.touch.px >= 18 &&
                input.touch.px <= 302) {
                const int ty = input.touch.py;
                if (ty >= 87 && ty <= 140) restart = true;
                if (ty >= 153 && ty <= 207) to_menu = true;
            }
            if (restart) start_new_game();
            else if (to_menu) {
                if (death_saved_) menu_active_ = true;
                else {
                    death_saved_ = write_scene_save();
                    if (death_saved_) {
                        has_saved_game_ = true;
                        menu_active_ = true;
                    }
                }
            }
            return;
        }
        // START is a physical 3DS pause control. Keep survival, AI,
        // crafting timers and autosave time frozen while the menu is up.
        if (input.pressed & KEY_START) paused_ = !paused_;
        if (paused_) {
            const bool pressed = input.touching && !touch_down_;
            touch_down_ = input.touching;
            if (input.pressed & KEY_B) paused_ = false;
            if (pressed && input.touch.px >= 20 && input.touch.px <= 300) {
                const int y = input.touch.py;
                if (y >= 52 && y <= 102) {
                    paused_ = false;
                } else if (y >= 109 && y <= 157) {
                    pause_save_attempted_ = true;
                    pause_save_feedback_ = write_scene_save();
                    has_saved_game_ = has_saved_game_ || pause_save_feedback_;
                } else if (y >= 164 && y <= 216) {
                    pause_save_attempted_ = true;
                    pause_save_feedback_ = write_scene_save();
                    has_saved_game_ = has_saved_game_ || pause_save_feedback_;
                    if (pause_save_feedback_) {
                        paused_ = false;
                        menu_active_ = true;
                    }
                }
            }
            return;
        }
        pause_save_feedback_ = false;
        pause_save_attempted_ = false;
        // The original map has bridge elements which connect its later
        // procedurally generated worlds. On a 3DS, A near a source bridge
        // platform initiates travel without interfering with inventory use.
        if ((input.pressed & KEY_A) && selected_inventory_slot_ < 0 &&
            near_source_bridge_exit() && world_level_ < 5) {
            start_next_source_map();
            return;
        }
        if (pending_craft_recipe_ < minidayz::items::kSourceCraftRecipeCount) {
            pending_craft_seconds_ -= std::max(0.0f, dt);
            if (pending_craft_seconds_ <= 0.0f) {
                const bool crafted = minidayz::items::craft_from_inventory(
                    inventory_, ground_loot_, player_.state().x,
                    player_.state().y, pending_craft_recipe_);
                pending_craft_recipe_ = minidayz::items::kSourceCraftRecipeCount;
                pending_craft_seconds_ = 0.0f;
                crafting_feedback_ = crafted
                    ? CraftFeedback::success : CraftFeedback::missing_or_full;
                crafting_feedback_elapsed_ = 0.0f;
                if (crafted) stage2_audio_.play_crafting();
            }
        }
        if (selected_inventory_slot_ >= 0 && (input.pressed & (KEY_A | KEY_X))) {
            const auto selected_index = static_cast<std::size_t>(selected_inventory_slot_);
            const auto* item = inventory_.slot(selected_index);
            const bool deploy_storage = item && !item->empty() &&
                (item->item_id == minidayz::items::kOriginalCivilianTentItemId ||
                 item->item_id == minidayz::items::kOriginalSuppliesStashItemId) &&
                (input.pressed & KEY_X) != 0;
            if (deploy_storage) {
                (void)deploy_source_storage(selected_index);
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
            const auto* drop_item = inventory_.slot(
                static_cast<std::size_t>(selected_inventory_slot_));
            const std::uint16_t item_id = drop_item ? drop_item->item_id : 0;
            if (ground_loot_.drop_from_inventory(
                    inventory_, static_cast<std::size_t>(selected_inventory_slot_),
                    0, player_.state().x, player_.state().y)) {
                play_original_drop_audio(item_id);
                selected_inventory_slot_ = -1;
            }
        }
        if ((input.pressed & KEY_R) &&
            ground_loot_.pickup_nearest(
                inventory_, player_.state().x, player_.state().y, 36.0f))
            stage2_audio_.play_item_pickup();
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
            const auto* source_recipe =
                minidayz::items::source_craft_recipe(recipe);
            if (source_recipe &&
                pending_craft_recipe_ >= minidayz::items::kSourceCraftRecipeCount) {
                if (source_recipe->delay_seconds > 0.0f) {
                    if (minidayz::items::can_craft_from_inventory(
                            inventory_, ground_loot_, player_.state().x,
                            player_.state().y, recipe)) {
                        // Original Craft_tent and Craft_xiaoyinqi include
                        // a five-second wait before the result action.
                        // Commit only on completion, without consuming
                        // ingredients on cancelled native sessions.
                        pending_craft_recipe_ = recipe;
                        pending_craft_seconds_ = source_recipe->delay_seconds;
                        crafting_feedback_ = CraftFeedback::pending;
                    } else {
                        crafting_feedback_ = CraftFeedback::missing_or_full;
                    }
                } else {
                    const bool crafted = minidayz::items::craft_from_inventory(
                        inventory_, ground_loot_, player_.state().x,
                        player_.state().y, recipe);
                    crafting_feedback_ = crafted
                        ? CraftFeedback::success : CraftFeedback::missing_or_full;
                    if (crafted) stage2_audio_.play_crafting();
                }
                crafting_feedback_elapsed_ = 0.0f;
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
            const auto storage = nearest_source_storage();
            if (input.touch.py >= 84 && input.touch.py < 99) {
                if (input.touch.px < 160) {
                    const std::size_t pages =
                        (inventory_.capacity() + kInventoryPageSize - 1) / kInventoryPageSize;
                    inventory_page_ = (inventory_page_ + 1) % pages;
                } else {
                    const std::size_t storage_slots = storage.stash
                        ? minidayz::items::kSourceSuppliesStashSlots
                        : minidayz::items::kNativeTentStorageSlots;
                    const std::size_t pages =
                        (storage_slots + kInventoryPageSize - 1) / kInventoryPageSize;
                    storage_page_ = (storage_page_ + 1) % pages;
                }
            } else if (storage.found &&
                       input.touch.py >= 99 && input.touch.py < 219) {
                const int col = input.touch.px < 160
                    ? (input.touch.px - 8) / 30 : (input.touch.px - 166) / 30;
                const int row = (input.touch.py - 99) / 40;
                if (col >= 0 && col < 5 && row >= 0 && row < 3) {
                    const std::size_t cell = static_cast<std::size_t>(row * 5 + col);
                    if (input.touch.px >= 8 && input.touch.px < 158) {
                        if (storage.stash)
                            (void)stashes_.store_from_inventory(
                                inventory_, inventory_page_ * kInventoryPageSize + cell,
                                storage.index);
                        else
                            (void)tents_.store_from_inventory(
                                inventory_, inventory_page_ * kInventoryPageSize + cell,
                                storage.index);
                    } else if (input.touch.px >= 166 && input.touch.px < 316) {
                        if (storage.stash)
                            (void)stashes_.take_to_inventory(
                                inventory_,storage.index,
                                storage_page_ * kInventoryPageSize + cell);
                        else
                            (void)tents_.take_to_inventory(
                                inventory_,storage.index,
                                storage_page_ * kInventoryPageSize + cell);
                    }
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
            input.touch.py >= 114 && input.touch.py < 127) {
            const int action = input.touch.px / 64;
            if (action == 3) {
                (void)ground_loot_.pickup_nearest(
                    inventory_, player_.state().x, player_.state().y, 36.0f);
            } else if (selected_inventory_slot_ >= 0) {
                const auto selected = static_cast<std::size_t>(selected_inventory_slot_);
                const auto* item = inventory_.slot(selected);
                if (item && !item->empty()) {
                    if (action == 0 || action == 1) {
                        if (action == 1 &&
                            (item->item_id == minidayz::items::kOriginalCivilianTentItemId ||
                             item->item_id == minidayz::items::kOriginalSuppliesStashItemId))
                            (void)deploy_source_storage(selected);
                        else
                            (void)minidayz::items::use_item_from_inventory(
                                inventory_, selected, player_, preview_rules_,
                                action == 1, perks_.item_use_flags(), &item_use_runtime_);
                    } else if (action == 2) {
                        const std::uint16_t item_id = item->item_id;
                        if (ground_loot_.drop_from_inventory(
                                inventory_, selected, 0, player_.state().x,
                                player_.state().y))
                            play_original_drop_audio(item_id);
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
                    if (backpack != 0 && destination < minidayz::items::kInventorySlotCount &&
                        inventory_.unequip_backpack_to_slot(destination))
                        stage2_audio_.play_backpack_zip();
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
                        const auto equipment_index = static_cast<std::size_t>(slot);
                        const std::size_t destination = first_available_inventory_slot(
                            equipped, equipment_.snapshot().conditions[equipment_index],
                            equipment_.fraction_snapshot().fractional_loss[equipment_index]);
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
                if (inventory_.repair_equipped_backpack_from_inventory_slot(source_slot)) {
                    selected_inventory_slot_ = -1;
                } else if (inventory_.equip_backpack_from_slot(source_slot)) {
                    stage2_audio_.play_backpack_zip();
                    selected_inventory_slot_ = -1;
                }
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
                    if (item->item_id == minidayz::items::kOriginalCivilianTentItemId ||
                        item->item_id == minidayz::items::kOriginalSuppliesStashItemId)
                        (void)deploy_source_storage(source);
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
        if (crafting_feedback_ != CraftFeedback::none &&
            crafting_feedback_ != CraftFeedback::pending) {
            crafting_feedback_elapsed_ += std::max(0.0f, dt);
            if (crafting_feedback_elapsed_ >= 2.5f) {
                crafting_feedback_ = CraftFeedback::none;
                crafting_feedback_elapsed_ = 0.0f;
            }
        }
        // Original Construct Timer_ALL advances once per 1.5 seconds,
        // toggling the day/night source cold branch at its real boundary.
        (void)day_night_.update(std::max(0.0f, dt));
        stage2_audio_.set_environment_ambience(
                    !day_night_.day_mode(),world_level_==4);
        // Original Rain_speed=360s is armed in Start events 8520771380755414.
        // CurrentLevel=0 sets rains_level=2; single-player is never inRaid.
        // choose(1,2,3,4)>1 starts rain; a later timer tick stops it.
        std::uint8_t rain_choice=1;
        const float rain_dt=std::clamp(dt,0.0f,1.0f);
        if (rain_timer_.snapshot().elapsed_seconds + rain_dt >=
            minidayz::items::SourceRainTimer::kOriginalRainSeconds) {
            const auto firing=rain_timer_.snapshot().timer_firings;
            std::mt19937 rng(world_seed_ ^ (0x9e3779b9u + firing));
            rain_choice=static_cast<std::uint8_t>(1u+(rng()%4u));
        }
        // The root Start events source starts the Check_loot timer with
        // LOOT_RESPAWN_TIME=1480s, separately from 360s Rain.
        // An actual Global_respawn dispatch additionally requires original
        // Map layout, onBridge, marker and per-location event ancestry.
        // The native timer is persisted but never bypasses those guards.
        if (loot_timer_.advance(rain_dt) && world_ready_) {
            // Original Check_loot -> Global_respawn -> Trigger_spawn(UID)
            // only targets source category 15 Map markers away from player.
            // Do not invent markers when the original location loader has
            // not produced any valid in-world marker instances.
            const auto firing = loot_timer_.snapshot().firings;
            std::mt19937 rng(world_seed_ ^ (0x6c6f6f74u + firing));
            minidayz::items::OriginalTimedLootContext context{};
            context.player_x = player_.state().x;
            context.player_y = player_.state().y;
            context.on_map_layout = true;
            context.on_bridge = false; // Map preview cannot enter Bridge layout.
            context.check_loot_timer_fired = true;
            context.seed_pubg = 0; // Original normal single-player source branch.
            for (std::size_t i = 0; i < world_.placement_count(); ++i) {
                const auto* placement = world_.placement(i);
                if (!placement || placement->object != 218 ||
                    !world_.bounds().contains({placement->x, placement->y}))
                    continue;
                std::uint16_t category = 0;
                if (!minidayz::items::source_authored_map_loot_marker(
                        placement->source_uid, category))
                    continue;
                minidayz::items::OriginalTimedLootMarker marker{};
                marker.x = placement->x;
                marker.y = placement->y;
                marker.category = category;
                // The authored Map marker is exact source object 218, not
                // source family 995; unknown generated markers are rejected.
                (void)minidayz::items::source_timed_loot_respawn(
                    ground_loot_, marker, context, rng(), rng());
            }
        }
        const bool was_raining=rain_timer_.active();
        (void)rain_timer_.update(rain_dt,kSourceDefaultRainsLevel,false,rain_choice);
        if (was_raining != rain_timer_.active()) {
            source_temperature_initialized_=false;
            if (rain_timer_.active()) stage2_audio_.start_rain_loop(kSourceDefaultRainsLevel==2);
            else stage2_audio_.stop_rain_loop();
        }
        const float current_source_warmth = equipment_.temperature_offset();
        const auto snowborn_level =
            perks_.level(minidayz::items::SourcePerk::snowborn);
        if (!source_temperature_initialized_ ||
            current_source_warmth != last_source_warmth_ ||
            snowborn_level != last_source_snowborn_level_ ||
            day_night_.day_mode() != last_source_day_mode_) {
            float source_temperature = 0.0f;
            if (minidayz::items::source_weather_temperature_with_snowborn(
                    minidayz::SurvivalDifficulty::regular,
                    rain_timer_.active()
                        ? minidayz::items::SourceColdPhase::rain
                        : day_night_.day_mode()
                            ? minidayz::items::SourceColdPhase::day
                            : minidayz::items::SourceColdPhase::night,
                    current_source_warmth,
                    minidayz::items::SourceRainColdBranch::full,
                    snowborn_level, source_temperature))
                source_weather_exposure_ = source_temperature;
            last_source_warmth_ = current_source_warmth;
            last_source_snowborn_level_ = snowborn_level;
            last_source_day_mode_ = day_night_.day_mode();
            source_temperature_initialized_ = true;
        }
        const float old_x=player_.state().x, old_y=player_.state().y;
        player_.update_movement(movement, dt, preview_rules_,
                                &WorldPreviewScene::blocked, this);
        // Play the original material-specific WAV only after moving.
        const float dx=player_.state().x-old_x;
        const float dy=player_.state().y-old_y;
        const float walked=std::hypot(dx,dy);
        if (walked>0.001f && std::isfinite(walked)) {
            // Game_events original AltMove footstep branches use the
            // Construct Every 0.25 seconds condition, not a guessed
            // number of movement pixels between samples.
            constexpr float kSourceFootstepSeconds=0.25f;
            footstep_elapsed_+=std::max(0.0f,dt);
            if (footstep_elapsed_>=kSourceFootstepSeconds) {
                footstep_elapsed_=std::fmod(
                    footstep_elapsed_,kSourceFootstepSeconds);
                stage2_audio_.play_footstep(
                    terrain_surface_at(player_.state().x,player_.state().y));
            }
        } else {
            footstep_elapsed_=0.0f;
        }
        player_.update_survival(dt, preview_rules_);
        // Exact original Player stats -> States five-second event
        // (UID 2665582317889917). This world preview has no original
        // tutorial UI, so Tutor_hint_* guards remain zero while playing.
        source_body_tick_elapsed_ += std::max(0.0f, dt);
        while (source_body_tick_elapsed_ >= 5.0f) {
            source_body_tick_elapsed_ -= 5.0f;
            (void)player_.apply_source_body_temperature_tick(
                source_weather_exposure_);
        }
        item_use_runtime_.update(dt, player_,
            &WorldPreviewScene::award_source_experience, &perks_);
        if (npc_ready_) npcs_.update(dt, player_.state().x, player_.state().y,
                                     &WorldPreviewScene::blocked, this);
        save_elapsed_ += dt;
        if (save_elapsed_ >= 2.0f) {
            save_elapsed_ = 0.0f;
            (void)write_scene_save();
        }
    }

    void render_bottom(minidayz::Renderer& renderer) noexcept override {
        if (loading_world_) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255));
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.95f);
            renderer.draw_text("GENERATING WORLD", 45.0f, 52.0f, 0.52f);
            static constexpr const char* steps[] = {
                "GENERATING GRID AND ROADS", "BUILDING MAP MARKERS",
                "PLACING SOURCE LOCATIONS", "ADDING BUNKERS",
                "ADDING BRIDGES", "FINALIZING WORLD"
            };
            const auto phase = world_.generation_phase();
            // Generation phases start at 1; phase 1 generates the road grid.
            const auto step = phase > 0 && phase <= 6 ? phase - 1 : 0;
            renderer.draw_text(steps[step], 26.0f, 112.0f, 0.32f);
            renderer.draw_solid_rect(24.0f, 169.0f, 272.0f, 16.0f,
                                     C2D_Color32(20, 28, 23, 255));
            renderer.draw_solid_rect(27.0f, 172.0f,
                266.0f * static_cast<float>(phase) / 6.0f, 10.0f,
                C2D_Color32(121, 156, 93, 255));
            renderer.draw_text("PLEASE KEEP THE GAME OPEN", 41.0f, 206.0f, 0.28f);
            return;
        }
        if (menu_active_) {
            // Top screen carries the original logo; the original wooden
            // menu buttons and labels belong on the 3DS touch screen.
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255));
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.95f);
            renderer.draw_text("SELECT GAME", 18.0f, 20.0f, 0.48f);
            const auto& cont = minidayz::menu::kContinueButton;
            const auto& start = minidayz::menu::kNewGameButton;
            if (has_saved_game_) {
                renderer.draw_solid_rect(cont.x(), cont.y(), cont.width(),
                    cont.height(), C2D_Color32(62, 92, 66, 255));
                (void)renderer.draw_menu_art(1, cont.x(), cont.y(),
                    cont.width(), cont.height());
                renderer.draw_text("CONTINUE", 92.0f, 96.0f, 0.55f);
            } else {
                renderer.draw_solid_rect(cont.x(), cont.y(), cont.width(),
                    cont.height(), C2D_Color32(55, 65, 58, 255));
                renderer.draw_text("NO SAVE FOUND", 70.0f, 96.0f, 0.45f);
            }
            renderer.draw_solid_rect(start.x(), start.y(),
                start.width(), start.height(),
                C2D_Color32(110, 75, 49, 255));
            (void)renderer.draw_menu_art(1, start.x(), start.y(),
                start.width(), start.height());
            renderer.draw_text("NEW GAME", 92.0f, 165.0f, 0.55f);
            char skin_label[64]{};
            std::snprintf(skin_label,sizeof(skin_label),
                "< SKIN %u/%u: %s >",
                static_cast<unsigned>(selected_skin_+1),
                static_cast<unsigned>(minidayz::stage1::kOriginalPlayerSkinCount),
                minidayz::stage1::kOriginalPlayerSkins[selected_skin_].name);
            renderer.draw_text(skin_label, 14.0f, 186.0f, 0.32f);
            if (generation_failed_)
                renderer.draw_text("WORLD GENERATION FAILED - RETRY NEW GAME",
                                   18.0f, 204.0f, 0.27f);
            renderer.draw_text(has_saved_game_ ? "A: Continue   X: New Game" :
                               "A/X: New Game", 18.0f, 216.0f, 0.32f);
            return;
        }
        if (paused_) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255));
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.95f);
            renderer.draw_text("PAUSED", 108.0f, 18.0f, 0.60f);
            constexpr float y[] = {52.0f, 109.0f, 164.0f};
            for (float top : y) {
                renderer.draw_solid_rect(20.0f, top, 280.0f, 48.0f,
                                         C2D_Color32(93, 73, 45, 255));
                (void)renderer.draw_menu_art(1, 20.0f, top, 280.0f, 48.0f);
            }
            renderer.draw_text("RESUME", 115.0f, 67.0f, 0.47f);
            renderer.draw_text("SAVE GAME", 91.0f, 124.0f, 0.47f);
            renderer.draw_text("MAIN MENU", 86.0f, 180.0f, 0.47f);
            if (pause_save_attempted_) {
                renderer.draw_text(pause_save_feedback_ ? "GAME SAVED" : "SAVE FAILED",
                                   110.0f, 220.0f, 0.28f);
            }
            return;
        }
        if (!player_.state().alive) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(18, 22, 21, 255));
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.95f);
            renderer.draw_text("SURVIVAL ENDED", 66.0f, 26.0f, 0.51f);
            renderer.draw_solid_rect(18.0f, 87.0f, 284.0f, 53.0f,
                                     C2D_Color32(90, 67, 44, 255));
            (void)renderer.draw_menu_art(1, 18.0f, 87.0f, 284.0f, 53.0f);
            (void)renderer.draw_source_sprite_fitted(
                minidayz::stage1::kOriginalRestartButtonArt.object,
                minidayz::stage1::kOriginalRestartButtonArt.animation,
                minidayz::stage1::kOriginalRestartButtonArt.frame,
                26.0f, 93.0f, 39.0f, 39.0f);
            renderer.draw_text("NEW GAME", 93.0f, 105.0f, 0.49f);
            renderer.draw_solid_rect(18.0f, 153.0f, 284.0f, 54.0f,
                                     C2D_Color32(57, 69, 56, 255));
            (void)renderer.draw_menu_art(1, 18.0f, 153.0f, 284.0f, 54.0f);
            renderer.draw_text("MAIN MENU", 91.0f, 172.0f, 0.49f);
            renderer.draw_text("X: RESTART  Y: MENU", 63.0f, 219.0f, 0.29f);
            return;
        }
        // Native 3DS inventory: use the actual Construct GUI frames
        // from assets/gfx, scaled to the 320x240 touch display.
        // Never use placeholder rectangles when the source sprite exists.
        const auto ui_art = [&](const minidayz::stage1::OriginalUiArt& art,
                                float x, float y, float w, float h,
                                float depth = 0.78f) noexcept {
            return renderer.draw_source_sprite_fitted(art.object,
                art.animation, art.frame, x, y, w, h, depth);
        };
        if (!ui_art(minidayz::stage1::kOriginalInventoryBackgroundArt,
                    0.0f, 0.0f, 320.0f, 240.0f, 0.98f)) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(70, 60, 48, 255));
        }
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
        // Show the ACTIVE original-source exposure, not the day-only
        // expression during rain or nighttime. Original cold branches apply
        // different rules to clothing warmth; the already-computed
        // source_weather_exposure_ preserves those source conditions.
        const char* weather_phase = rain_timer_.active()
            ? "RAIN" : day_night_.day_mode() ? "DAY" : "NIGHT";
        char cold_status[64]{};
        std::snprintf(cold_status, sizeof(cold_status),
            "%s  EXPOSURE %.1f  WARMTH %.1f",
            weather_phase,
            static_cast<double>(source_weather_exposure_),
            static_cast<double>(equipment_.temperature_offset()));
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
        // Original MiniDAYZ parchment equipment cards, reflowed for
        // the 3DS bottom touchscreen. Each card preserves the source
        // image proportions and shows its original item sprite/condition.
        if (!storage_open_ && !crafting_open_ && !perks_open_ && !details_open_) {
            static constexpr const char* equipment_labels[] = {
                "HELM", "COAT", "VEST", "PANTS", "HAND", "PACK", "SATCH"
            };
            for (std::size_t i = 0; i < 7; ++i) {
                const float x = 6.0f + static_cast<float>(i % 4) * 77.0f;
                const float y = 40.0f + static_cast<float>(i / 4) * 38.0f;
                const auto& frame =
                    i == 1 ? minidayz::stage1::kOriginalEquipmentCoatArt
                    : i == 2 ? minidayz::stage1::kOriginalEquipmentVestArt
                    : i == 5 ? minidayz::stage1::kOriginalEquipmentBackpackArt
                    : i == 6 ? minidayz::stage1::kOriginalEquipmentSatchelArt
                    : minidayz::stage1::kOriginalInventoryCardArt;
                if (!ui_art(frame, x, y, 76.0f, 36.0f))
                    renderer.draw_solid_rect(x, y, 76.0f, 36.0f,
                                             C2D_Color32(125, 111, 84, 255));
                const auto item_id =
                    i == 5 ? inventory_.equipped_backpack()
                    : i == 6 ? inventory_.equipped_satchel()
                    : equipment_.equipped(static_cast<minidayz::items::EquipmentSlot>(i));
                const auto condition =
                    i == 5 ? inventory_.equipped_backpack_condition()
                    : i == 6 ? inventory_.equipped_satchel_condition()
                    : equipment_.snapshot().conditions[i];
                if (item_id != 0) {
                    (void)renderer.draw_item_icon(item_id, x + 17.0f,
                                                  y + 18.0f, 29.0f, 29.0f);
                    const auto fraction = i < 5
                        ? equipment_.fraction_snapshot().fractional_loss[i] : 0u;
                    const float exact_condition =
                        static_cast<float>(condition) -
                        static_cast<float>(fraction) / 100.0f;
                    const char* source_name = minidayz::items::source_item_name(item_id);
                    if (source_name && *source_name) {
                        char short_name[15]{};
                        std::snprintf(short_name, sizeof(short_name),
                                      "%.13s", source_name);
                        renderer.draw_text(short_name, x + 34.0f, y + 8.0f, 0.17f);
                    } else {
                        renderer.draw_text(equipment_labels[i],
                                           x + 34.0f, y + 8.0f, 0.19f);
                    }
                    char durability[16]{};
                    std::snprintf(durability, sizeof(durability), "%.0f%%",
                                  static_cast<double>(exact_condition));
                    renderer.draw_text(durability, x + 34.0f, y + 21.0f, 0.22f);
                    renderer.draw_solid_rect(x + 34.0f, y + 32.0f,
                                             38.0f, 2.0f,
                                             C2D_Color32(44, 40, 30, 255));
                    renderer.draw_solid_rect(x + 34.0f, y + 32.0f,
                        38.0f * std::clamp(exact_condition / 100.0f, 0.0f, 1.0f),
                        2.0f, C2D_Color32(177, 146, 79, 255));
                } else {
                    renderer.draw_text(equipment_labels[i],
                                       x + 29.0f, y + 14.0f, 0.30f);
                }
            }
            static constexpr const char* actions[] = {
                "USE", "ALT", "DROP", "PICK", "SPLIT"
            };
            for (int i = 0; i < 5; ++i) {
                const float x = static_cast<float>(i * 64);
                renderer.draw_solid_rect(x + 1.0f, 115.0f, 62.0f, 11.0f,
                                         C2D_Color32(90, 76, 55, 255));
                renderer.draw_text(actions[i], x + 13.0f, 117.0f, 0.27f);
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
            const auto storage = nearest_source_storage();
            const auto* tent = storage.found && !storage.stash
                ? tents_.tent(storage.index) : nullptr;
            const auto* stash = storage.found && storage.stash
                ? stashes_.stash(storage.index) : nullptr;
            if (!storage.found) {
                renderer.draw_text("NO STORAGE NEARBY", 34.0f, 130.0f, 0.36f);
                renderer.draw_text("DEPLOY TENT OR SUPPLIES STASH FROM BAG", 12.0f, 153.0f, 0.23f);
            } else {
                char labels[64]{};
                std::snprintf(labels, sizeof(labels),
                    storage.stash ? "BAG %u     SUPPLIES STASH %u" :
                        "BAG %u    TAP TO PAGE    TENT %u",
                    static_cast<unsigned>(inventory_page_ + 1),
                    static_cast<unsigned>(storage_page_ + 1));
                renderer.draw_text(labels, 12.0f, 87.0f, 0.28f);
                for (int row = 0; row < 3; ++row) {
                    for (int col = 0; col < 5; ++col) {
                        const std::size_t offset = static_cast<std::size_t>(row * 5 + col);
                        const auto* carried = inventory_.slot(
                            inventory_page_ * kInventoryPageSize + offset);
                        const auto tent_slot = storage_page_ * kInventoryPageSize + offset;
                        const auto* stored = storage.stash
                            ? (stash && tent_slot < stash->items.size()
                                ? &stash->items[tent_slot] : nullptr)
                            : (tent && tent_slot < tent->items.size()
                                ? &tent->items[tent_slot] : nullptr);
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
                renderer.draw_text(
                    success ? "CRAFTED - CHECK BAG OR GROUND"
                    : crafting_feedback_ == CraftFeedback::pending
                        ? "CRAFTING - WAIT FOR ORIGINAL SOURCE DELAY"
                        : "NEED MATERIALS OR OUTPUT SPACE",
                    18.0f, 211.0f, 0.19f);
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
                const unsigned exact_hundredths =
                    static_cast<unsigned>(selected->condition) * 100u -
                    static_cast<unsigned>(selected->fractional_loss);
                std::snprintf(heading, sizeof(heading),
                    "ITEM %u x%u CONDITION %u.%02u%%",
                    static_cast<unsigned>(selected->item_id),
                    static_cast<unsigned>(selected->count),
                    exact_hundredths / 100u, exact_hundredths % 100u);
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
        // 5 x 3 source-sprite backpack cells on the 3DS touch screen.
        const int page_start = static_cast<int>(inventory_page_ * kInventoryPageSize);
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 5; ++col) {
                const int index = page_start + row * 5 + col;
                const float x = 10.0f + col * 60.0f;
                const float y = 130.0f + row * 31.0f;
                const bool selected = index == selected_inventory_slot_;
                const bool restricted = index >= 0 &&
                    inventory_.is_exclusive_satchel_slot(
                        static_cast<std::size_t>(index));
                if (!ui_art(minidayz::stage1::kOriginalSlotBackgroundArt,
                            x, y, 56.0f, 29.0f))
                    renderer.draw_solid_rect(x, y, 56.0f, 29.0f,
                        C2D_Color32(129, 115, 91, 255));
                if (selected) {
                    const u32 accent = C2D_Color32(224, 187, 87, 255);
                    renderer.draw_solid_rect(x, y, 56.0f, 2.0f, accent);
                    renderer.draw_solid_rect(x, y + 27.0f, 56.0f, 2.0f, accent);
                    renderer.draw_solid_rect(x, y, 2.0f, 29.0f, accent);
                    renderer.draw_solid_rect(x + 54.0f, y, 2.0f, 29.0f, accent);
                }
                if (restricted) {
                    renderer.draw_solid_rect(x, y + 23.0f, 56.0f, 6.0f,
                        C2D_Color32(86, 112, 103, 190));
                }
                if (restricted) {
                    const auto satchel_id = inventory_.equipped_satchel();
                    const char* label =
                        (satchel_id == 459 || satchel_id == 753)
                            ? "ARMY" : "MED";
                    renderer.draw_text(label, x + 4.0f, y + 2.0f, 0.19f);
                }
                const auto* item = inventory_.slot(static_cast<std::size_t>(index));
                if (item && !item->empty()) {
                    // Use the exact item ID -> Construct object mapping from
                    // the original game's MDZItemTable.
                    (void)renderer.draw_item_icon(item->item_id,
                        x + 28.0f, y + 12.0f, 29.0f, 19.0f);
                    char quantity[8]{};
                    std::snprintf(quantity, sizeof(quantity), "x%u",
                                  static_cast<unsigned>(item->count));
                    renderer.draw_text(quantity, x + 3.0f, y + 20.0f, 0.21f);
                    if (item->condition > 0 && item->condition < 100) {
                        renderer.draw_solid_rect(x + 6.0f, y + 26.0f, 44.0f, 2.0f,
                                                 C2D_Color32(30, 30, 30, 255));
                        renderer.draw_solid_rect(x + 6.0f, y + 26.0f,
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
        if(near_source_bridge_exit()){
            renderer.draw_solid_rect(18.0f, 104.0f, 284.0f, 21.0f,
                                     C2D_Color32(39, 63, 46, 230));
            renderer.draw_text("A: TRAVEL TO NEXT MAP",
                               48.0f, 108.0f, 0.37f);
        }
    }

    void render(minidayz::Renderer& renderer) noexcept override {
        if (loading_world_) {
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f, 0.0f, 400.0f, 240.0f,
                                     C2D_Color32(24, 34, 29, 255));
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 400.0f, 240.0f, 0.95f);
            if (!renderer.draw_menu_art(0, 42.0f, 36.0f, 316.0f, 112.0f))
                renderer.draw_text("MINI DAYZ", 76.0f, 72.0f, 1.05f);
            renderer.draw_text("GENERATING NEW SURVIVAL WORLD", 51.0f, 198.0f, 0.37f);
            return;
        }
        if (menu_active_) {
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f, 0.0f, 400.0f, 240.0f,
                                     C2D_Color32(24, 34, 29, 255));
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 400.0f, 240.0f, 0.95f);
            if (!renderer.draw_menu_art(0, 42.0f, 36.0f, 316.0f, 112.0f))
                renderer.draw_text("MINI DAYZ", 76.0f, 72.0f, 1.05f);
            renderer.draw_text("3DS EDITION", 143.0f, 156.0f, 0.43f);
            (void)renderer.draw_source_animation(
                minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object,
                "idle_down",0,200.0f,193.0f,1.1f,1.1f,0.35f);
            renderer.draw_text("L/R: CHANGE PLAYER SKIN", 76.0f, 223.0f, 0.32f);
            return;
        }
        if (!player_.state().alive) {
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f, 0.0f, 400.0f, 240.0f,
                                     C2D_Color32(24, 20, 19, 255));
            if (!renderer.draw_source_sprite_fitted(
                    minidayz::stage1::kOriginalDeathScreenArt.object,
                    minidayz::stage1::kOriginalDeathScreenArt.animation,
                    minidayz::stage1::kOriginalDeathScreenArt.frame,
                    0.0f, 0.0f, 400.0f, 240.0f, 0.95f))
                (void)renderer.draw_menu_art(
                    2, 0.0f, 0.0f, 400.0f, 240.0f, 0.95f);
            renderer.draw_text("YOU DIED", 132.0f, 86.0f, 0.85f);
            const char* reason = "SURVIVOR LOST";
            switch (player_.state().death_reason) {
            case minidayz::PlayerDeathReason::hunger:
                reason = "STARVATION"; break;
            case minidayz::PlayerDeathReason::thirst:
                reason = "DEHYDRATION"; break;
            case minidayz::PlayerDeathReason::cold:
                reason = "HYPOTHERMIA"; break;
            default: break;
            }
            renderer.draw_text(reason, 124.0f, 151.0f, 0.40f);
            renderer.draw_text("CHOOSE AN OPTION ON THE TOUCH SCREEN",
                               38.0f, 215.0f, 0.30f);
            return;
        }
        if (paused_) {
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f, 0.0f, 400.0f, 240.0f,
                                     C2D_Color32(24, 34, 29, 255));
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 400.0f, 240.0f, 0.95f);
            if (!renderer.draw_menu_art(0, 42.0f, 27.0f, 316.0f, 112.0f))
                renderer.draw_text("MINI DAYZ", 76.0f, 68.0f, 1.05f);
            renderer.draw_text("GAME PAUSED", 128.0f, 162.0f, 0.60f);
            renderer.draw_text("START OR B: RESUME", 114.0f, 208.0f, 0.32f);
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
        // The source Map uses Construct Tilemaps, not sprites. The native
        // floor must be drawn from the original ground_lvl1 tile sheet,
        // before all world objects, or the entire terrain looks blank.
        // Source WorldGenerationProfile levels 4/5 use a 33-row map,
        // with the second terrain half beginning at grid row 18.
        const float source_second_half_y=world_.grid().height()==33
            ? world_.bounds().height*18.0f/33.0f : 1.0e30f;
        if (!renderer.draw_original_ground_floor(
                camera_x, camera_y, view_w, view_h,
                world_.generation_level(),source_second_half_y)) {
            renderer.draw_solid_rect(camera_x, camera_y, view_w, view_h,
                                     C2D_Color32(80, 72, 62, 255));
            renderer.draw_text("ORIGINAL FLOOR TEXTURE FAILED TO LOAD",
                               camera_x + 12.0f, camera_y + 12.0f, 0.30f);
        }

        if (world_ready_) {
            // WorldGrid codes 12/13/14 are the original procedural road
            // network. They must be drawn before buildings and trees, or
            // even a correctly loaded ground sheet shows an empty wilderness.
            if (!renderer.draw_source_road_grid(world_.grid(),world_.bounds(),
                    camera_x,camera_y,view_w,view_h,world_.generation_level())) {
                renderer.draw_text("ORIGINAL ROAD TILES FAILED TO LOAD",
                                   camera_x+12.0f,camera_y+26.0f,0.30f);
            }
            // Tilemap objects are not sprite objects. Without their exact
            // original RLE cells, beaches/water/forests appear empty even
            // if all PNG files are successfully converted to 3DS textures.
            if (!renderer.draw_original_environment_tilemaps(
                    camera_x,camera_y,view_w,view_h)) {
                renderer.draw_text("ORIGINAL ENVIRONMENT TILES FAILED TO LOAD",
                                   camera_x+12.0f,camera_y+40.0f,0.30f);
            }
            // Original Game_events scatter source Tilemap object 230 per
            // generated location cell. These are real 30px sprites from
            // assets/gfx/ground_enviroment_tilemap.png, not placeholder art.
            if (!renderer.draw_source_procedural_environment(world_,
                    camera_x,camera_y,view_w,view_h)) {
                renderer.draw_text("PROCEDURAL TERRAIN TILES FAILED",
                                   camera_x+12.0f,camera_y+54.0f,0.30f);
            }
            // Render the actual source Map layout in world coordinates.
            // Cull placements outside the camera rectangle to keep 3DS draw
            // cost bounded instead of drawing the entire 36k x 20k layout.
            const auto* source_map = minidayz::stage1::find_layout("Map");
            const auto source_map_index = source_map
                ? static_cast<std::uint16_t>(
                      source_map - minidayz::stage1::kLayoutTable)
                : 0;
            for (std::size_t i = 0; i < world_.placement_count(); ++i) {
                const auto* placement = world_.placement(i);
                if (!placement || placement->layer >= 44) continue;
                // The original Map layers 44+ contain Rain/NVG/night,
                // browser/mobile GUI controls, and event triggers. These
                // cannot be painted as static world sprites on the 3DS:
                // weather has separate runtime state and UI lives below.
                // Original source Tilemaps, Arrays and invisible trigger
                // objects must not be drawn as bogus marker rectangles.
                const auto* source_object =
                    minidayz::stage1::find_object(placement->object);
                if (!source_object || source_object->type != 18) continue;
                const auto* source_layer = source_map
                    ? minidayz::stage1::find_layer(
                          source_map_index, placement->layer)
                    : nullptr;
                if (source_layer && !source_layer->visible) continue;
                if (source_layer &&
                    (std::strstr(source_layer->name, "respawn") ||
                     std::strstr(source_layer->name, "_resp") ||
                     std::strstr(source_layer->name, "points"))) continue;
                // Original UI player sprite belongs to runtime-controlled
                // survivor rendering below, not a permanent source Map image.
                if (placement->layer == 32 || placement->opacity <= 0.0f)
                    continue;
                const float px = placement->x;
                const float py = placement->y;
                const float sx = placement->scale_x != 0.0f
                    ? placement->scale_x : 1.0f;
                const auto key=std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{
                    placement->object,0,placement->source_frame};
                const auto* begin=minidayz::stage1::kSpriteFrames;
                const auto* end=begin+minidayz::stage1::kSpriteFrameCount;
                const auto* sprite=std::lower_bound(begin,end,key,
                    [](const minidayz::stage1::SpriteFrameDef& item,
                       const auto& wanted){
                        return std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{
                            item.object,item.animation,item.frame}<wanted;
                    });
                if (sprite!=end && sprite->object==placement->object &&
                    sprite->animation==0 && sprite->frame==placement->source_frame) {
                    if (!minidayz::terrain::source_sprite_in_view(
                        px,py,sprite->width,sprite->height,
                        sprite->origin_x,sprite->origin_y,sx,sx,
                        placement->angle*0.01745329251994329577f,
                        camera_x,camera_y,view_w,view_h)) continue;
                } // Keep unknown frame dimensions visible for diagnostics.
                // Source Map layer 32 is player, with Buildings_bg (31)
                // below it and Buildings/Trees (41-43) above it. Preserve
                // those layers through Citro2D's depth ordering instead of
                // drawing every building behind the player.
                const float layer_depth = std::clamp(
                    0.90f - 0.0125f * static_cast<float>(placement->layer),
                    0.10f, 0.90f);
                if (!renderer.draw_source_sprite(placement->object, 0, placement->source_frame,
                        px, py, sx, sx, layer_depth,
                        placement->angle * 0.01745329251994329577f,
                        placement->opacity)) {
                    // Original art was not loaded: do not draw a fake object.
                    // Missing source atlas frames are audited during CI.
                }
            }

            // Source bridge and bunker spawn events create terrain features
            // that are NOT pre-placed Map instances. Draw their native
            // Construct sprites at the extracted world coordinates.
            for (std::size_t i = 0; i < world_.feature_count(); ++i) {
                const auto* feature = world_.feature(i);
                if (!feature) continue;
                const bool bridge = feature->kind ==
                    minidayz::WorldFeatureKind::Bridge &&
                    (feature->variant & 1u) != 0u;
                const bool bunker = feature->kind ==
                    minidayz::WorldFeatureKind::Bunker;
                if (!bridge && !bunker) continue;
                const auto* object =
                    minidayz::stage1::find_object(feature->source_object);
                if (!object || object->type != 18) continue;
                const float x = feature->position.x;
                const float y = feature->position.y;
                if (x < camera_x - 128.0f || x > camera_x + view_w + 128.0f ||
                    y < camera_y - 128.0f || y > camera_y + view_h + 128.0f)
                    continue;
                (void)renderer.draw_source_sprite(
                    feature->source_object, 0, 0, x, y,
                    1.0f, 1.0f, 0.58f);
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

        for (std::size_t i = 0; i < stashes_.count(); ++i) {
            const auto* stash = stashes_.stash(i);
            if (!stash || stash->x < camera_x - 24.0f ||
                stash->x > camera_x + view_w + 24.0f ||
                stash->y < camera_y - 24.0f ||
                stash->y > camera_y + view_h + 24.0f) continue;
            (void)renderer.draw_item_icon(
                minidayz::items::kOriginalSuppliesStashItemId,
                stash->x, stash->y, 28.0f, 28.0f);
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

        const char* skin_direction="down";
        switch(p.facing){
        case minidayz::PlayerFacing::down: skin_direction="down"; break;
        case minidayz::PlayerFacing::up: skin_direction="up"; break;
        case minidayz::PlayerFacing::left: skin_direction="left"; break;
        case minidayz::PlayerFacing::right: skin_direction="right"; break;
        }
        char skin_animation[24]{};
        std::snprintf(skin_animation,sizeof(skin_animation),"%s_%s",
            p.animation==minidayz::PlayerAnimation::idle?"idle":"run",
            skin_direction);
        const bool sprite_drawn=p.alive &&
            renderer.draw_source_animation(
                minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object,
                skin_animation,player_.source_animation_frame(),
                p.x,p.y,0.55f,0.55f);
        if (p.alive) {
            // Original item pickups and wearable actor overlays are NOT
            // interchangeable. Only draw original animation poses proven to
            // exist for this source item. The selected skin supplies the
            // player hotspot; all original equipment layers use that hotspot.
            const bool moving=p.animation!=minidayz::PlayerAnimation::idle;
            const auto draw_equipped_layer=[&](std::uint16_t item_id,
                                               bool weapon,float depth) {
                if(item_id==0)return;
                const auto* visual=minidayz::stage1::original_equipment_visual(item_id);
                const auto* prefix=minidayz::stage1::original_equipment_pose(
                    visual,moving,weapon);
                if(!visual||!prefix)return;
                char name[32]{};
                std::snprintf(name,sizeof(name),"%s_%s",prefix,skin_direction);
                (void)renderer.draw_source_animation(visual->object,name,
                    player_.source_animation_frame(),p.x,p.y,0.55f,0.55f,depth);
            };
            // Source-authored equipment back-to-front. The originals use
            // separate layers on top of the chosen character base skin.
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::pants),false,0.44f);
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::outerwear),false,0.43f);
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::vest),false,0.42f);
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::head),false,0.41f);
            draw_equipped_layer(inventory_.equipped_backpack(),false,0.40f);
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::hands),true,0.39f);
        }
        if (!sprite_drawn) {
            renderer.draw_solid_rect(p.x - 5.0f, p.y - 5.0f, 10.0f, 12.0f,
                                     p.alive ? C2D_Color32(190, 126, 65, 255)
                                             : C2D_Color32(130, 45, 45, 255));
        }

        // The original Map has separate night (46) and Rain (44) layers.
        // They are runtime effects, not static map placements. Draw the
        // actual Construct overlay art only in the corresponding source
        // day/night and rain state, fitted to the 400x240 upper display.
        if (!day_night_.day_mode()) {
            (void)renderer.draw_source_sprite_fitted(
                minidayz::stage1::kOriginalNightOverlayArt.object,
                minidayz::stage1::kOriginalNightOverlayArt.animation,
                minidayz::stage1::kOriginalNightOverlayArt.frame,
                camera_x, camera_y, view_w, view_h, 0.16f);
        }
        if (rain_timer_.active()) {
            (void)renderer.draw_source_sprite_fitted(
                minidayz::stage1::kOriginalRainFadeArt.object,
                minidayz::stage1::kOriginalRainFadeArt.animation,
                minidayz::stage1::kOriginalRainFadeArt.frame,
                camera_x, camera_y, view_w, view_h, 0.14f);
        }
    }

private:
    void play_original_drop_audio(std::uint16_t item_id) noexcept {
        if (minidayz::items::is_backpack_item(item_id)) {
            stage2_audio_.play_equipment_drop();
            return;
        }
        minidayz::items::EquipmentSlot source_slot{};
        if (minidayz::items::EquipmentSystem::source_slot_for_item(
                item_id, source_slot) &&
            (source_slot == minidayz::items::EquipmentSlot::outerwear ||
             source_slot == minidayz::items::EquipmentSlot::vest ||
             source_slot == minidayz::items::EquipmentSlot::pants)) {
            stage2_audio_.play_equipment_drop();
            return;
        }
        stage2_audio_.play_item_drop();
    }

    struct StorageTarget {
        std::size_t index{};
        bool stash{};
        bool found{};
    };

    StorageTarget nearest_source_storage() const noexcept {
        const auto x = player_.state().x;
        const auto y = player_.state().y;
        const auto tent_index = tents_.nearest(x,y,36.0f);
        const auto stash_index = stashes_.nearest(x,y,36.0f);
        const auto* tent = tents_.tent(tent_index);
        const auto* stash = stashes_.stash(stash_index);
        if (!tent && !stash) return {};
        if (!tent) return {stash_index,true,true};
        if (!stash) return {tent_index,false,true};
        const auto tx = tent->x-x, ty = tent->y-y;
        const auto sx = stash->x-x, sy = stash->y-y;
        return sx*sx+sy*sy <= tx*tx+ty*ty
            ? StorageTarget{stash_index,true,true}
            : StorageTarget{tent_index,false,true};
    }

    bool deploy_source_storage(std::size_t slot) noexcept {
        const auto* item = inventory_.slot(slot);
        if (!item || item->empty()) return false;
        if (item->item_id == minidayz::items::kOriginalCivilianTentItemId) {
            if (stashes_.nearest(player_.state().x, player_.state().y, 16.0f) <
                stashes_.count()) return false;
            return tents_.deploy_from_inventory_checked(
                inventory_, slot, player_.state().x, player_.state().y,
                &WorldPreviewScene::blocked, this);
        }
        if (item->item_id == minidayz::items::kOriginalSuppliesStashItemId) {
            if (tents_.nearest(player_.state().x, player_.state().y, 16.0f) <
                tents_.tent_count()) return false;
            return stashes_.deploy_from_inventory_checked(
                inventory_, slot, player_.state().x, player_.state().y,
                &WorldPreviewScene::blocked, this);
        }
        return false;
    }

    static bool award_source_experience(void* context,
                                        std::uint32_t amount) noexcept {
        auto* perks = static_cast<minidayz::items::SourcePerkSystem*>(context);
        return perks && perks->award_experience(amount);
    }

    static constexpr std::size_t kInventoryPageSize = 15;
    static constexpr std::size_t kCraftPageSize = 3;

    static int equipment_slot_at(int x, int y) noexcept {
        // Original parchment cards reflow to four 76x36 cards per row
        // on the 320x240 touchscreen, instead of seven 43px blank boxes.
        if (y < 40 || y >= 114 || x < 6 || x >= 314) return -1;
        const int row = (y - 40) / 38;
        const int col = (x - 6) / 77;
        const int index = row * 4 + col;
        return row >= 0 && row < 2 && col >= 0 && col < 4 &&
               index < 7 ? index : -1;
    }

    std::size_t first_available_inventory_slot(std::uint16_t item_id,
                                               std::uint8_t condition,
                                               std::uint8_t fractional_loss) const noexcept {
        const auto limit = minidayz::items::source_max_stack(item_id);
        std::size_t first_empty = inventory_.capacity();
        for (std::size_t i = 0; i < inventory_.capacity(); ++i) {
            if (!inventory_.slot_accepts(i, item_id)) continue;
            const auto* item = inventory_.slot(i);
            if (!item) continue;
            if (item->empty()) {
                if (first_empty == inventory_.capacity()) first_empty = i;
            } else if (item->item_id == item_id &&
                       item->condition == condition &&
                       item->fractional_loss == fractional_loss &&
                       item->count < limit) {
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
        if (x < 10 || x >= 310 || y < 130 || y >= 223) return -1;
        const int col = (x - 10) / 60;
        const int row = (y - 130) / 31;
        return col >= 0 && col < 5 && row >= 0 && row < 3
            ? static_cast<int>(page * kInventoryPageSize) + row * 5 + col
            : -1;
    }

    bool near_source_bridge_exit() const noexcept {
        if (!world_ready_ || world_level_ >= 5) return false;
        const auto& player = player_.state();
        for(std::size_t i=0;i<world_.feature_count();++i){
            const auto* feature=world_.feature(i);
            if (!feature ||
                feature->kind!=minidayz::WorldFeatureKind::Bridge ||
                (feature->variant & 1u)==0u ||
                feature->source_object != 318)continue;
            const float dx=player.x-feature->position.x;
            const float dy=player.y-feature->position.y;
            if(dx*dx+dy*dy<100.0f*100.0f)return true;
        }
        return false;
    }

    void start_next_source_map() noexcept {
        if(world_level_>=5)return;
        // A new map is a new Construct world generation, not simply a
        // background texture swap. Preserve survivor condition and items.
        // The source bridge artwork remains the travel landmark on 3DS.
        ++world_level_;
        stage2_audio_.stop_environment_ambience();
        stage2_audio_.stop_rain_loop();
        footstep_elapsed_=0.0f;
        world_seed_ ^= (0x9e3779b9u + static_cast<std::uint32_t>(world_level_) * 0x85ebca6bu);
        if(world_seed_==0)world_seed_=0x6d696e69u;
        ground_loot_.clear();
        tents_.clear();
        stashes_.clear();
        selected_inventory_slot_=-1;
        storage_open_=false;
        crafting_open_=false;
        perks_open_=false;
        details_open_=false;
        paused_=false;
        new_game_generation_=true;
        travel_generation_=true;
        begin_world_generation();
    }

    void start_new_game() noexcept {
        world_seed_ = make_new_world_seed();
        stage2_audio_.stop_environment_ambience();
        stage2_audio_.stop_rain_loop();
        footstep_elapsed_=0.0f;
        world_level_ = 0;
        travel_generation_ = false;
        player_ = minidayz::PlayerSystem(make_initial_state());
        player_.set_source_skin(
            minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
        inventory_.clear();
        equipment_.clear();
        ground_loot_.clear();
        tents_.clear();
        stashes_.clear();
        day_night_.reset();
        rain_timer_.reset();
        loot_timer_.reset();
        item_use_runtime_.clear();
        perks_.clear();
        source_temperature_initialized_ = false;
        source_weather_exposure_ = 0.0f;
        source_body_tick_elapsed_ = 0.0f;
        last_source_warmth_ = 0.0f;
        last_source_snowborn_level_ = 0;
        inventory_page_ = 0;
        storage_page_ = 0;
        crafting_page_ = 0;
        perk_page_ = 0;
        crafting_feedback_ = CraftFeedback::none;
        crafting_feedback_elapsed_ = 0.0f;
        pending_craft_recipe_ = minidayz::items::kSourceCraftRecipeCount;
        pending_craft_seconds_ = 0.0f;
        storage_open_ = false;
        crafting_open_ = false;
        perks_open_ = false;
        details_open_ = false;
        tap_was_selected_ = false;
        selected_inventory_slot_ = -1;
        save_elapsed_ = 0.0f;
        new_game_generation_ = true;
        death_saved_ = false;
        paused_ = false;
        has_saved_game_ = false; // Keep old disk save intact if generation fails.
        begin_world_generation();
    }

    // 3DS safety fallback until original player-spawn events are fully ported:
    // choose a traversable position in the source-generated settlement grid,
    // not the fixed 160,120 point near an empty corner of the large Map.
    bool place_new_player_on_generated_map() noexcept {
        const auto& grid = world_.grid();
        if(grid.width()==0 || grid.height()==0) return false;
        const float cw = world_.bounds().width / grid.width();
        const float ch = world_.bounds().height / grid.height();
        const std::size_t total = static_cast<std::size_t>(grid.width()) * grid.height();
        const std::size_t first = world_seed_ % total;
        constexpr float offsets[][2] = {
            {0.0f,0.0f}, {180.0f,0.0f}, {-180.0f,0.0f},
            {0.0f,180.0f}, {0.0f,-180.0f},
            {280.0f,160.0f}, {-280.0f,160.0f},
            {280.0f,-160.0f}, {-280.0f,-160.0f},
            {400.0f,0.0f}, {-400.0f,0.0f}
        };
        for(std::size_t i=0;i<total;++i) {
            const std::size_t index=(first+i)%total;
            const int x=static_cast<int>(index%grid.width());
            const int y=static_cast<int>(index/grid.width());
            // Only select a cell containing a source-generated location.
            if(grid.at(x,y)==0) continue;
            const float center_x=(static_cast<float>(x)+0.5f)*cw;
            const float center_y=(static_cast<float>(y)+0.5f)*ch;
            for(const auto& offset:offsets) {
                const float px=center_x+offset[0],py=center_y+offset[1];
                if(px<16.0f || py<16.0f ||
                   px>world_.bounds().width-16.0f ||
                   py>world_.bounds().height-16.0f) continue;
                if(blocked(this,px,py) ||
                   blocked(this,px-6.0f,py-8.0f) ||
                   blocked(this,px+6.0f,py-8.0f) ||
                   blocked(this,px-6.0f,py+8.0f) ||
                   blocked(this,px+6.0f,py+8.0f)) continue;
                // A bridge transition retains original survivor stats,
                // injuries, hunger, equipment and perk progression.
                auto state=travel_generation_ ? player_.state() : make_initial_state();
                state.x=px;
                state.y=py;
                player_=minidayz::PlayerSystem(state);
                player_.set_source_skin(
                    minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
                return true;
            }
        }
        return false;
    }

    void begin_world_generation() noexcept {
        generation_failed_ = false;
        world_ready_ = false;
        npc_ready_ = false;
        menu_active_ = true;
        loading_world_ = world_.begin_source_generation(world_seed_, "Map", world_level_);
        if (!loading_world_) generation_failed_ = true;
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

    minidayz::terrain::FootstepSurface terrain_surface_at(
        float x,float y) const noexcept {
        const auto surface=terrain_collision_.surface_at(x,y);
        if (surface!=minidayz::terrain::FootstepSurface::ground)
            return surface;
        const auto& grid=world_.grid();
        const auto& bounds=world_.bounds();
        if(grid.width()==0||grid.height()==0||
           bounds.width<=0.0f||bounds.height<=0.0f||x<0.0f||y<0.0f||
           x>=bounds.width||y>=bounds.height)
            return surface;
        const auto col=static_cast<int>(
            x*static_cast<float>(grid.width())/bounds.width);
        const auto row=static_cast<int>(
            y*static_cast<float>(grid.height())/bounds.height);
        if(minidayz::terrain::generated_road_cell(grid.at(col,row)))
            return minidayz::terrain::FootstepSurface::road;
        // The fifth original ground sheet is snow and has its own
        // snow_run WAV set; don't use ordinary hard-earth footsteps there.
        if(world_level_==4)
            return minidayz::terrain::FootstepSurface::snow;
        return surface;
    }

    static bool blocked(void* context, float x, float y) noexcept {
        const auto* scene = static_cast<const WorldPreviewScene*>(context);
        if (!scene || !std::isfinite(x) || !std::isfinite(y) ||
            x < 0.0f || y < 0.0f ||
            x > scene->world_.bounds().width ||
            y > scene->world_.bounds().height)
            return true;
        const auto* map_layout = minidayz::stage1::find_layout("Map");
        if (!map_layout) return true;
        const auto map_index = static_cast<std::uint16_t>(
            map_layout - minidayz::stage1::kLayoutTable);
        // Original Construct instance positions are relative to their
        // sprite origins, not always their upper-left corners.
        if (scene->terrain_collision_.blocked(x,y)) return true;
        for (std::size_t i = 0; i < scene->world_.placement_count(); ++i) {
            const auto* placement = scene->world_.placement(i);
            // A tilemap's rectangular bounds mostly contain EMPTY cells.
            // Only actual original Solid cells (above) are obstacles.
            if (placement) {
                const auto* kind= minidayz::stage1::find_object(placement->object);
                if (kind && kind->type==16) continue;
            }
            if (!placement || placement->width <= 0.0f ||
                placement->height <= 0.0f) continue;
            const auto* layer = minidayz::stage1::find_layer(
                map_index, placement->layer);
            if (!layer) continue;
            const bool building = std::strcmp(layer->name, "Buildings") == 0;
            const bool barrier = std::strcmp(layer->name, "Obstacles") == 0 ||
                                 std::strcmp(layer->name, "Water") == 0;
            if (!building && !barrier) continue;
            // Match rotated original house and barrier sprite bounds;
            // a static axis-aligned box blocks empty corners and doorways.
            if (minidayz::terrain::source_solid_sprite_contains(
                    placement->x,placement->y,
                    placement->width,placement->height,
                    placement->origin_x,placement->origin_y,
                    placement->angle,x,y))
                return true;
        }
        return false;
    }

    minidayz::WorldSystem world_{};
    minidayz::terrain::SolidTerrain terrain_collision_{};
    minidayz::NpcSystem npcs_{};
    std::uint32_t world_seed_{};
    std::uint8_t world_level_{};
    bool travel_generation_{};
    float save_elapsed_{};
    float footstep_elapsed_{};
    bool world_ready_{};
    bool npc_ready_{};
    bool menu_active_{true};
    bool paused_{};
    bool death_saved_{};
    bool pause_save_feedback_{};
    bool pause_save_attempted_{};
    bool loading_world_{};
    bool new_game_generation_{};
    bool generation_failed_{};
    bool has_saved_game_{};
    minidayz::PlayerSystem player_;
    std::size_t selected_skin_{}; // Default original skin 679, first in generated catalog.
    minidayz::items::InventorySystem inventory_{};
    minidayz::items::EquipmentSystem equipment_{};
    minidayz::items::GroundLootSystem ground_loot_{};
    minidayz::items::TentStorageSystem tents_{};
    minidayz::items::StashStorageSystem stashes_{};
    minidayz::items::SourceDayNightClock day_night_{};
    minidayz::items::SourceRainTimer rain_timer_{};
    minidayz::items::SourceLootTimer loot_timer_{};
    minidayz::items::ItemUseRuntime item_use_runtime_{};
    minidayz::items::SourcePerkSystem perks_{};
    minidayz::Stage2Audio stage2_audio_{};
    minidayz::PlayerRules preview_rules_{};
    bool source_temperature_initialized_{};
    float source_weather_exposure_{};
    float source_body_tick_elapsed_{};
    float last_source_warmth_{};
    std::uint8_t last_source_snowborn_level_{};
    bool last_source_day_mode_{true};
    int selected_inventory_slot_{-1};
    int drag_source_slot_{-1};
    int drag_target_slot_{-1};
    int drag_target_equipment_slot_{-1};
    std::size_t inventory_page_{};
    std::size_t storage_page_{};
    std::size_t crafting_page_{};
    std::size_t perk_page_{};
    enum class CraftFeedback : std::uint8_t { none, success, missing_or_full, pending };
    CraftFeedback crafting_feedback_{CraftFeedback::none};
    float crafting_feedback_elapsed_{};
    std::size_t pending_craft_recipe_{minidayz::items::kSourceCraftRecipeCount};
    float pending_craft_seconds_{};
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
