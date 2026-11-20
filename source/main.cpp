#include "minidayz/engine.hpp"
#include "minidayz/boot_menu.hpp"
#include "minidayz/menu_preferences.hpp"
#include "minidayz/generated/menu_source_catalog.hpp"
#include "minidayz/player.hpp"
#include "minidayz/world.hpp"
#include "minidayz/source_map_overlay.hpp"
#include "minidayz/terrain_collision.hpp"
#include "minidayz/terrain_obstacle_index.hpp"
#include "minidayz/terrain_visible_index.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include "minidayz/terrain_source_visibility.hpp"
#include "minidayz/generated/sprite_catalog.hpp"
#include "minidayz/generated/player_skin_source.hpp"
#include "minidayz/generated/player_spawn_source.hpp"
#include "minidayz/player_equipment_visual.hpp"
#include "minidayz/npc_system.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/inventory.hpp"
#include "minidayz/inventory_touch_layout.hpp"
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
#include <random>
#include <vector>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <memory>
#include <new>
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
        menu_selection_=minidayz::menu::default_selection(has_saved_game_);
        minidayz::menu::MenuPreferences stored{};
        if(minidayz::menu::load_menu_preferences(stored)){
            selected_skin_=stored.skin_index;
            difficulty_index_=stored.difficulty;
            selected_difficulty_=static_cast<minidayz::SurvivalDifficulty>(stored.difficulty);
            stage2_audio_.set_muted(stored.muted!=0);
        }
        player_.set_source_skin(
            minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
        if (restored && !inventory_.restore_with_satchel(restored->inventory, restored->satchel))
            inventory_.clear();
        if (restored && (!equipment_.restore_with_fraction(
                restored->equipment, restored->equipment_fraction) ||
                !equipment_.restore_quick_pistol(restored->quick_pistol)))
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
        preview_rules_ = minidayz::source_survival_rules(selected_difficulty_);
        // Game_events/Animations Set AltMove MaxSpeed source action
        // 156884066447061 uses base value 100 before clothing modifiers.
        // Old 72/108 numbers were invented preview speed values.
        preview_rules_.walk_speed = 100.0f;
        preview_rules_.run_speed = 100.0f;
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

    // Continue MUST load the persisted record at selection time.
    // Without this, attempting a new world first mutates the in-memory
    // player/seed and a later Continue incorrectly resumes the new run.
    bool restore_continue_from_disk() noexcept {
        std::unique_ptr<SavedGame> restored(new(std::nothrow) SavedGame{});
        if(!restored || !load_saved_game(*restored)) {
            has_saved_game_=false;
            menu_selection_=minidayz::menu::default_selection(false);
            return false;
        }
        world_seed_=restored->seed;
        world_level_=restored->world_level;
        player_=minidayz::PlayerSystem(restored->player);
        player_.set_source_skin(
            minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
        if(!inventory_.restore_with_satchel(
                restored->inventory,restored->satchel))return false;
        if(!equipment_.restore_with_fraction(
                restored->equipment,restored->equipment_fraction) ||
           !equipment_.restore_quick_pistol(restored->quick_pistol))return false;
        if(!ground_loot_.restore(restored->ground_loot))return false;
        if(!tents_.restore(restored->tents))return false;
        if(!stashes_.restore(restored->stashes))return false;
        if(!day_night_.restore(restored->source_time))return false;
        if(!rain_timer_.restore(restored->source_rain))return false;
        if(!loot_timer_.restore(restored->source_loot))return false;
        if(!item_use_runtime_.restore(restored->pending_item_use))return false;
        if(!perks_.restore(restored->perks))return false;
        pending_craft_recipe_=restored->pending_craft.recipe_index;
        pending_craft_seconds_=restored->pending_craft.seconds_remaining;
        crafting_feedback_=pending_craft_recipe_<minidayz::items::kSourceCraftRecipeCount
            ?CraftFeedback::pending:CraftFeedback::none;
        new_game_generation_=false;
        travel_generation_=false;
        source_temperature_initialized_=false;
        last_source_warmth_=equipment_.temperature_offset();
        last_source_snowborn_level_=
            perks_.level(minidayz::items::SourcePerk::snowborn);
        inventory_page_=0;
        selected_inventory_slot_=-1;
        paused_=false;
        return true;
    }

    void persist_menu_settings() noexcept {
        minidayz::menu::MenuPreferences prefs{};
        prefs.skin_index=static_cast<std::uint8_t>(selected_skin_);
        prefs.difficulty=static_cast<std::uint8_t>(difficulty_index_);
        prefs.muted=stage2_audio_.muted()?1u:0u;
        (void)minidayz::menu::save_menu_preferences(prefs);
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
                menu_active_ = true;
                menu_page_ = minidayz::menu::BootPage::main;
                menu_selection_ = minidayz::menu::default_selection(has_saved_game_);
                boot_input_gate_.reset();
                return;
            }
            if (world_.generation_complete()) {
                loading_world_ = false;
                world_ready_ = true;
                collision_index_.rebuild(world_);
                visibility_index_.rebuild(world_);
                npc_ready_ = npcs_.initialize(world_);
                if (new_game_generation_ && !place_new_player_on_generated_map()) {
                    // Do not start a new run at a hard-coded, blocked map edge.
                    loading_world_ = false;
                    world_ready_ = false;
                    generation_failed_ = true;
                    menu_active_ = true;
                    menu_page_ = minidayz::menu::BootPage::main;
                    menu_selection_ = minidayz::menu::default_selection(has_saved_game_);
                    boot_input_gate_.reset();
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
            // Ignore a stale launcher/overlay A/X press until all boot
            // controls have been released; a new press is then required.
            const bool menu_controls_held=
                (input.held & (KEY_A|KEY_B|KEY_X|KEY_Y|KEY_L|
                               KEY_R|KEY_TOUCH|KEY_DUP|KEY_DDOWN))!=0 ||
                input.touching;
            if(!boot_input_gate_.update(dt,menu_controls_held)){
                touch_down_=input.touching;
                return;
            }
            const bool tap=input.touching && !touch_down_;
            touch_down_=input.touching;
            const auto go_main=[&]()noexcept {
                menu_page_=minidayz::menu::BootPage::main;
                menu_selection_=minidayz::menu::default_selection(has_saved_game_);
            };
            if(menu_page_!=minidayz::menu::BootPage::main){
                if((input.pressed & KEY_B) ||
                    (tap && minidayz::menu::kSubpageBack.contains(
                        input.touch.px,input.touch.py))){
                    if(menu_detail_open_ &&
                       (menu_page_==minidayz::menu::BootPage::achievements ||
                        menu_page_==minidayz::menu::BootPage::unlocks)){
                        menu_detail_open_=false;
                        menu_detail_scroll_=0;
                    }else{
                        menu_detail_open_=false;
                        go_main();
                    }
                    return;
                }
                if(menu_page_==minidayz::menu::BootPage::character){
                    const auto choose_skin=[&](minidayz::menu::SkinAction action)noexcept {
                        selected_skin_=minidayz::menu::advance_skin(
                            selected_skin_,minidayz::stage1::kOriginalPlayerSkinCount,
                            action);
                        persist_menu_settings();
                    };
                    if(input.pressed & KEY_L)
                        choose_skin(minidayz::menu::SkinAction::previous);
                    if(input.pressed & KEY_R)
                        choose_skin(minidayz::menu::SkinAction::next);
                    if(tap)choose_skin(minidayz::menu::skin_touch_action(
                        input.touch.px,input.touch.py));
                    if(input.pressed & KEY_A)go_main();
                } else if(menu_page_==minidayz::menu::BootPage::difficulty){
                    if(input.pressed & KEY_DUP)
                        difficulty_index_=(difficulty_index_+2)%3;
                    if(input.pressed & KEY_DDOWN)
                        difficulty_index_=(difficulty_index_+1)%3;
                    int chosen=-1;
                    if(input.pressed & KEY_A)chosen=static_cast<int>(difficulty_index_);
                    if(tap)chosen=minidayz::menu::difficulty_at(
                        input.touch.px,input.touch.py);
                    if(chosen>=0 && chosen<3){
                        difficulty_index_=static_cast<std::size_t>(chosen);
                        selected_difficulty_=static_cast<minidayz::SurvivalDifficulty>(chosen);
                        preview_rules_=minidayz::source_survival_rules(selected_difficulty_);
                        // Original AltMove max-speed action is 100 before equipment effects.
                        preview_rules_.walk_speed=100.0f;
                        preview_rules_.run_speed=100.0f;
                        preview_rules_.collision_half_width=5.0f;
                        preview_rules_.collision_half_height=7.0f;
                        preview_rules_.min_temperature=-10.0f;
                        preview_rules_.max_temperature=10.0f;
                        persist_menu_settings();
                        start_new_game();
                    }
                } else if(menu_page_==minidayz::menu::BootPage::achievements ||
                          menu_page_==minidayz::menu::BootPage::unlocks){
                    // Original Menu_Events has separate Character and
                    // Weapon_unlocks groups; X switches their source pages.
                    if(menu_page_==minidayz::menu::BootPage::unlocks &&
                       (input.pressed & KEY_X)){
                        unlock_weapons_page_=!unlock_weapons_page_;
                        menu_list_index_=0;
                        menu_detail_open_=false;
                        menu_detail_scroll_=0;
                        return;
                    }
                    if(menu_detail_open_){
                        if((input.pressed & (KEY_DUP|KEY_L)) && menu_detail_scroll_>0)
                            --menu_detail_scroll_;
                        if((input.pressed & (KEY_DDOWN|KEY_R)) && menu_detail_scroll_<32)
                            ++menu_detail_scroll_;
                        if(tap && input.touch.py>=40 && input.touch.py<206)
                            menu_detail_scroll_=std::min<std::size_t>(32,
                                menu_detail_scroll_+1);
                        if(input.pressed & KEY_A){
                            menu_detail_open_=false;
                            menu_detail_scroll_=0;
                        }
                        return;
                    }
                    const auto count=menu_page_==minidayz::menu::BootPage::achievements
                        ?minidayz::menu::kSourceAchievementCount
                        :unlock_weapons_page_
                            ?minidayz::menu::kSourceWeaponUnlockCount
                            :minidayz::menu::kSourceUnlockCount;
                    if(input.pressed & KEY_DUP && menu_list_index_>0)
                        --menu_list_index_;
                    if(input.pressed & KEY_DDOWN && menu_list_index_+1<count)
                        ++menu_list_index_;
                    if(input.pressed & KEY_L)
                        menu_list_index_=menu_list_index_>=4?menu_list_index_-4:0;
                    if(input.pressed & KEY_R)
                        menu_list_index_=std::min(count-1,menu_list_index_+4);
                    if(tap && input.touch.py>=43 && input.touch.py<163){
                        const auto row=(input.touch.py-43)/30;
                        const auto offset=(menu_list_index_/4)*4;
                        if(offset+static_cast<std::size_t>(row)<count){
                            menu_list_index_=offset+static_cast<std::size_t>(row);
                            menu_detail_open_=true;
                            menu_detail_scroll_=0;
                        }
                    }
                    if(input.pressed & KEY_A){
                        menu_detail_open_=true;
                        menu_detail_scroll_=0;
                    }
                } else if(menu_page_==minidayz::menu::BootPage::options){
                    if((input.pressed & KEY_A) ||
                        (tap && input.touch.px>=64 && input.touch.px<=256 &&
                                input.touch.py>=70 && input.touch.py<=117)){
                        stage2_audio_.set_muted(!stage2_audio_.muted());
                        persist_menu_settings();
                        if(!stage2_audio_.muted() && world_ready_){
                            stage2_audio_.set_environment_ambience(
                                !day_night_.day_mode(),world_level_==4);
                            if(rain_timer_.active())
                                stage2_audio_.start_rain_loop(
                                    kSourceDefaultRainsLevel==2);
                        }
                    }
                }
                return;
            }
            if(input.pressed & KEY_DUP)
                menu_selection_=minidayz::menu::move_selection(
                    menu_selection_,-1,has_saved_game_);
            if(input.pressed & KEY_DDOWN)
                menu_selection_=minidayz::menu::move_selection(
                    menu_selection_,1,has_saved_game_);
            if(input.pressed & KEY_L)
                selected_skin_=minidayz::menu::advance_skin(
                    selected_skin_,minidayz::stage1::kOriginalPlayerSkinCount,
                    minidayz::menu::SkinAction::previous);
            if(input.pressed & KEY_R)
                selected_skin_=minidayz::menu::advance_skin(
                    selected_skin_,minidayz::stage1::kOriginalPlayerSkinCount,
                    minidayz::menu::SkinAction::next);
            if(input.pressed & (KEY_L|KEY_R))persist_menu_settings();
            if(input.pressed & KEY_X)
                menu_page_=minidayz::menu::BootPage::character;
            if(input.pressed & KEY_Y)
                menu_page_=minidayz::menu::BootPage::multiplayer;
            auto chosen=minidayz::menu::MenuEntry::none;
            if((input.pressed & KEY_A) &&
                menu_page_==minidayz::menu::BootPage::main)
                chosen=menu_selection_;
            if(tap){
                if(minidayz::menu::kCharacterButton.contains(
                    input.touch.px,input.touch.py))
                    menu_page_=minidayz::menu::BootPage::character;
                else if(minidayz::menu::kMultiplayerButton.contains(
                    input.touch.px,input.touch.py))
                    menu_page_=minidayz::menu::BootPage::multiplayer;
                else if(menu_page_==minidayz::menu::BootPage::main)
                    chosen=minidayz::menu::main_entry_at(
                        input.touch.px,input.touch.py);
            }
            switch(chosen){
            case minidayz::menu::MenuEntry::continue_game:
                // The disabled NO SAVE row must never report a failed world
                // generation when pressed directly on the touchscreen.
                if(!has_saved_game_)break;
                if(restore_continue_from_disk())
                    begin_world_generation();
                else
                    generation_failed_=true;
                break;
            case minidayz::menu::MenuEntry::new_game:
                // The original source does not start world generation
                // before the user chooses Novice/Regular/Veteran.
                menu_page_=minidayz::menu::BootPage::difficulty;
                break;
            case minidayz::menu::MenuEntry::achievements:
                menu_detail_open_=false;
                menu_list_index_=0;
                menu_page_=minidayz::menu::BootPage::achievements;
                break;
            case minidayz::menu::MenuEntry::unlocks:
                menu_detail_open_=false;
                unlock_weapons_page_=false;
                menu_list_index_=0;
                menu_page_=minidayz::menu::BootPage::unlocks;
                break;
            case minidayz::menu::MenuEntry::options:
                menu_page_=minidayz::menu::BootPage::options;
                break;
            case minidayz::menu::MenuEntry::none:
                break;
            }
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
                if (death_saved_) {
                    menu_active_ = true;
                    boot_input_gate_.reset();
                }
                else {
                    death_saved_ = write_scene_save();
                    if (death_saved_) {
                        has_saved_game_ = true;
                        menu_active_ = true;
                        boot_input_gate_.reset();
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
                        menu_page_=minidayz::menu::BootPage::main;
                        menu_selection_=minidayz::menu::default_selection(has_saved_game_);
                        boot_input_gate_.reset();
                    }
                }
            }
            return;
        }
        pause_save_feedback_ = false;
        pause_save_attempted_ = false;
        // 3DS Y opens a world map without replacing the source inventory.
        // B/Y or a lower-screen close tap returns to the backpack.
        // This screen is an adaptation of the original minimap/map display,
        // and MUST be compared to its Construct MiniMap_legend/GUI events.
        const bool map_button_tap=input.touching && !touch_down_ &&
            input.touch.px>=258 && input.touch.py<14;
        if((input.pressed & KEY_Y) || map_button_tap) {
            map_open_=!map_open_;
            if(map_button_tap)touch_down_=true;
        }
        if(map_open_) {
            const bool tap=input.touching && !touch_down_;
            touch_down_=input.touching;
            if((input.pressed & KEY_B) ||
               (tap && input.touch.py>=207))
                map_open_=false;
            return;
        }
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
                const auto hit=minidayz::items::ui::storage_at(
                    input.touch.px,input.touch.py);
                if(hit.valid){
                    const std::size_t cell=static_cast<std::size_t>(hit.local);
                    bool moved=false;
                    if(hit.carried){
                        if(storage.stash)
                            moved=stashes_.store_from_inventory(
                                inventory_, inventory_page_ * kInventoryPageSize + cell,
                                storage.index);
                        else
                            moved=tents_.store_from_inventory(
                                inventory_, inventory_page_ * kInventoryPageSize + cell,
                                storage.index);
                    }else{
                        if(storage.stash)
                            moved=stashes_.take_to_inventory(
                                inventory_,storage.index,
                                storage_page_ * kInventoryPageSize + cell);
                        else
                            moved=tents_.take_to_inventory(
                                inventory_,storage.index,
                                storage_page_ * kInventoryPageSize + cell);
                    }
                    if(moved)stage2_audio_.play_item_pickup();
                }
            }
        }
        // Source-backed item operations are accessible without physical
        // buttons. This narrow action strip stays on the lower touchscreen.
        if (!crafting_open_ && !storage_open_ && !perks_open_ && touch_pressed &&
            input.touch.py >= 25 && input.touch.py < 38) {
            details_open_ = !details_open_;
            details_scroll_=0;
        }
        if(details_open_ && touch_pressed &&
           input.touch.px>=8 && input.touch.px<312 &&
           input.touch.py>=94 && input.touch.py<220)
            details_scroll_+=3;
        if (!crafting_open_ && !storage_open_ && !perks_open_ && touch_pressed &&
            minidayz::items::ui::kActions.contains(
                input.touch.px,input.touch.py)) {
            const int action = minidayz::items::ui::action_at(
                input.touch.px,input.touch.py);
            if (action == 3) {
                if(ground_loot_.pickup_nearest(
                       inventory_, player_.state().x, player_.state().y, 36.0f))
                    stage2_audio_.play_item_pickup();
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
            drag_start_x_=input.touch.px;
            drag_start_y_=input.touch.py;
            drag_moved_=false;
            drag_source_slot_ = -1;
            drag_equipped_source_slot_ = -1;
            drag_target_slot_ = -1;
            drag_target_equipment_slot_ = -1;
            const int equipment_target = equipment_slot_at(input.touch.px, input.touch.py);
            if(equipment_target>=0){
                // Do not discard an equipped object immediately on touch
                // down. The source inventory can drag gear BACK into a
                // backpack; preserve the outgoing item/condition until
                // drop or tap release confirms the destination.
                const auto item_id=equipment_target==5
                    ? inventory_.equipped_backpack()
                    : equipment_target==6
                        ? inventory_.equipped_satchel()
                        : equipment_target==8
                            ? equipment_.quick_pistol()
                            : equipment_.equipped(
                                gear_slot_for_card(equipment_target));
                if(item_id!=0){
                    drag_equipped_source_slot_=equipment_target;
                    drag_target_equipment_slot_=equipment_target;
                    selected_equipment_slot_=equipment_target;
                    // Source supports simultaneously worn rifle and melee.
                    // Selecting either equipment card makes it the active
                    // original player hands/weapon appearance.
                    if(equipment_target==8)active_quick_pistol_=true;
                    else if(equipment_target==4 || equipment_target==7){
                        active_quick_pistol_=false;
                        active_held_slot_=gear_slot_for_card(equipment_target);
                    }
                    details_scroll_=0;
                }else selected_equipment_slot_=-1;
                selected_inventory_slot_=-1;
            }
            // Page through larger backpacks without changing the 5 x 3 grid.
            if (minidayz::items::ui::kPageNavigation.contains(
                    input.touch.px,input.touch.py)) {
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
                    selected_equipment_slot_=-1;
                    const auto* item = inventory_.slot(static_cast<std::size_t>(slot));
                    tap_was_selected_ = item && !item->empty() &&
                        selected_inventory_slot_ == slot;
                    selected_inventory_slot_ = item && !item->empty() ? slot : -1;
                    details_scroll_=0;
                    if (selected_inventory_slot_ >= 0) {
                        drag_source_slot_ = slot;
                        drag_target_slot_ = slot;
                    }
                }
            }
        }
        if (!crafting_open_ && !storage_open_ && !perks_open_ && !details_open_ &&
            input.touching && (drag_source_slot_>=0 || drag_equipped_source_slot_>=0)) {
            drag_touch_x_=input.touch.px;
            drag_touch_y_=input.touch.py;
            const int dx=input.touch.px-drag_start_x_;
            const int dy=input.touch.py-drag_start_y_;
            if(dx*dx+dy*dy>=36)drag_moved_=true;
            const int equipment_target = equipment_slot_at(input.touch.px, input.touch.py);
            if (equipment_target >= 0 &&
                equipment_target < static_cast<int>(minidayz::items::ui::kEquipment.size())) {
                drag_target_equipment_slot_ = equipment_target;
                drag_target_slot_ = -1;
            } else {
                drag_target_equipment_slot_ = -1;
                const int target = inventory_slot_at(input.touch.px, input.touch.py,
                                                    inventory_page_);
                // Clear stale drag targets whenever the stylus leaves a
                // valid cell. Otherwise releasing outside the backpack can
                // move an item into a slot the player is no longer touching.
                drag_target_slot_ = target;
            }
        }
        if (!crafting_open_ && !storage_open_ && !perks_open_ && !details_open_ && touch_released) {
            if(drag_equipped_source_slot_>=0){
                const int source=drag_equipped_source_slot_;
                const bool same_equipment=drag_target_equipment_slot_==source;
                // An equipped item can be dropped into a chosen free
                // backpack cell, or tapped to place it into the first
                // suitable free cell. Dropping on another equipment card
                // is not interpreted as a discard.
                std::size_t destination=inventory_.capacity();
                if(drag_moved_ && drag_target_slot_>=0)
                    destination=static_cast<std::size_t>(drag_target_slot_);
                // A stationary touch merely selects an equipment card.
                // Never silently unequip a helmet/backpack on a tap,
                // especially when its storage contains items.
                (void)same_equipment;
                bool moved=false;
                if(source==5 && destination<minidayz::items::kInventorySlotCount){
                    moved=inventory_.unequip_backpack_to_slot(destination);
                    if(moved)stage2_audio_.play_backpack_zip();
                }else if(source==6 && destination<minidayz::items::kInventorySlotCount){
                    moved=inventory_.unequip_satchel_to_slot(destination);
                }else if(source==8 && destination<inventory_.capacity()){
                    moved=equipment_.unequip_quick_pistol_to_inventory(
                        inventory_,destination);
                    if(moved)active_quick_pistol_=false;
                }else if((source>=0 && (source<5 || source==7)) && destination<inventory_.capacity()){
                    moved=equipment_.unequip_to_inventory(
                        inventory_,gear_slot_for_card(source),
                        destination);
                }
                if(moved){
                    selected_inventory_slot_=static_cast<int>(destination);
                    selected_equipment_slot_=-1;
                    if(source<5 || source==7 || source==8)
                        stage2_audio_.play_equipment_drop();
                }
            } else if (drag_source_slot_ >= 0 && drag_target_equipment_slot_ == 5) {
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
            } else if (drag_source_slot_ >= 0 &&
                       drag_target_equipment_slot_ == 8) {
                // MP Drop_pistol is independent of the source rifle/melee slots.
                // Accept only the exact handgun IDs from root mp_kit.js.
                if(equipment_.equip_quick_pistol_from_inventory(
                        inventory_,static_cast<std::size_t>(drag_source_slot_))){
                    selected_inventory_slot_=-1;
                    active_quick_pistol_=true;
                    stage2_audio_.play_equipment_drop();
                }
            } else if (drag_source_slot_ >= 0 && drag_target_equipment_slot_ >= 0) {
                const auto source_slot = static_cast<std::size_t>(drag_source_slot_);
                const auto target = gear_slot_for_card(drag_target_equipment_slot_);
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
                if (changed){
                    selected_inventory_slot_ = -1;
                    if(drag_target_equipment_slot_==4 ||
                       drag_target_equipment_slot_==7){
                        active_held_slot_=target;
                        active_quick_pistol_=false;
                    }
                }
            } else if (drag_source_slot_ >= 0 && drag_target_slot_ >= 0 &&
                       drag_target_slot_ != drag_source_slot_) {
                if (inventory_.move(static_cast<std::size_t>(drag_source_slot_),
                                    static_cast<std::size_t>(drag_target_slot_)))
                    selected_inventory_slot_ = drag_target_slot_;
            } else if (drag_source_slot_ >= 0 &&
                       drag_target_slot_ == drag_source_slot_ &&
                       tap_was_selected_ && !drag_moved_) {
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
            drag_moved_=false;
            drag_source_slot_ = -1;
            drag_equipped_source_slot_ = -1;
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
                    selected_difficulty_,
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

    void render_source_map(minidayz::Renderer& renderer) noexcept {
        renderer.reset_camera();
        const auto& grid=world_.grid();
        const auto& bounds=world_.bounds();
        renderer.draw_solid_rect(0.0f,0.0f,320.0f,240.0f,
            C2D_Color32(34,37,30,255),0.01f);
        // Use supplied original minimap GUI art where available. Never
        // substitute the inventory's cyan drag/drop selection image.
        (void)renderer.draw_original_image("minimap_tile-sheet0.png",
            7.0f,28.0f,306.0f,178.0f,0.05f);
        renderer.draw_solid_rect(7.0f,28.0f,306.0f,178.0f,
            C2D_Color32(41,53,41,238),0.11f);
        renderer.draw_text("WORLD MAP",107.0f,4.0f,0.55f);
        if(grid.width()==0 || grid.height()==0 ||
           !(bounds.width>0.0f&&bounds.height>0.0f)){
            renderer.draw_text("MAP NOT GENERATED",54.0f,103.0f,0.36f);
            return;
        }
        // The source Map-generation Array encodes road segments 12/13/14
        // and settlements with the original POI IDs. Show those cells
        // rather than inventing buildings or revealing ungenerated places.
        const minidayz::map_overlay::MapRect area{12.0f,32.0f,296.0f,170.0f};
        const float cw=area.width/static_cast<float>(grid.width());
        const float ch=area.height/static_cast<float>(grid.height());
        for(int y=0;y<grid.height();++y){
            for(int x=0;x<grid.width();++x){
                using minidayz::map_overlay::CellKind;
                const auto kind=minidayz::map_overlay::cell_kind(grid.at(x,y));
                const float xx=area.x+x*cw, yy=area.y+y*ch;
                const float roadw=std::max(1.0f,cw*0.34f);
                const float roadh=std::max(1.0f,ch*0.34f);
                const auto road_color=C2D_Color32(155,151,127,255);
                if(kind==CellKind::road_horizontal ||
                   kind==CellKind::road_crossing)
                    renderer.draw_solid_rect(xx,yy+(ch-roadh)*0.5f,
                        cw,roadh,road_color,0.35f);
                if(kind==CellKind::road_vertical ||
                   kind==CellKind::road_crossing)
                    renderer.draw_solid_rect(xx+(cw-roadw)*0.5f,yy,
                        roadw,ch,road_color,0.36f);
                if(minidayz::map_overlay::is_location(kind)){
                    const float size=std::max(3.0f,std::min(cw,ch)*0.8f);
                    const auto color=kind==CellKind::military
                        ? C2D_Color32(122,158,109,255)
                        :kind==CellKind::hospital
                            ? C2D_Color32(210,185,146,255)
                            : C2D_Color32(218,204,158,255);
                    renderer.draw_solid_rect(xx+(cw-size)*0.5f,
                        yy+(ch-size)*0.5f,size,size,color,0.52f);
                    // The source project includes an actual minimap
                    // location marker; keep its icon instead of replacing
                    // every settlement with a fabricated map symbol.
                    (void)renderer.draw_original_image(
                        "minimap_mark-sheet0.png",
                        xx+(cw-size)*0.5f,yy+(ch-size)*0.5f,
                        size,size,0.57f);
                }
            }
        }
        const auto point=minidayz::map_overlay::player_point(
            player_.state().x,player_.state().y,bounds,area);
        // Original minimap_player artwork accompanies the X used to show
        // the player's precise world position on the rendered map.
        (void)renderer.draw_original_image("minimap_player-sheet0.png",
            point.x-6.0f,point.y-6.0f,12.0f,12.0f,0.77f);
        renderer.draw_text("X",point.x-4.0f,point.y-9.0f,0.43f);
        renderer.draw_text("ROADS     LOCATIONS     X = YOU",
                           15.0f,207.0f,0.28f);
        renderer.draw_solid_rect(9.0f,222.0f,302.0f,16.0f,
            C2D_Color32(76,66,47,255),0.61f);
        renderer.draw_text("Y / B / TOUCH: BACK TO INVENTORY",
                           17.0f,224.0f,0.27f);
    }

    void render_bottom(minidayz::Renderer& renderer) noexcept override {
        if (loading_world_) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255),0.01f);
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.05f);
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
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f,0.0f,320.0f,240.0f,
                C2D_Color32(22,26,24,255),0.01f);
            (void)renderer.draw_menu_art(2,0.0f,0.0f,320.0f,240.0f,0.05f);
            renderer.draw_solid_rect(0.0f,0.0f,320.0f,240.0f,
                C2D_Color32(12,15,13,160),0.16f);
            // Reuse original menu subpage artwork wherever available.
            // Original PNG names resolve through the committed source atlas.
            if(menu_page_==minidayz::menu::BootPage::achievements ||
               menu_page_==minidayz::menu::BootPage::unlocks){
                (void)renderer.draw_original_image(
                    "menu_achieves_bg-sheet0.png",
                    0.0f,23.0f,320.0f,189.0f,0.19f);
            }else if(menu_page_==minidayz::menu::BootPage::options){
                (void)renderer.draw_original_image(
                    "options_menu-sheet0.png",
                    // Original Construct source sprite is 128x211.
                    // Preserve its aspect instead of stretching it 294x150.
                    96.0f,24.0f,128.0f,211.0f,0.19f);
            }
            const auto wood=[&](const minidayz::menu::TouchBounds& box,
                                bool selected,bool enabled) noexcept {
                renderer.draw_solid_rect(box.x(),box.y(),
                    box.width(),box.height(),enabled
                    ?C2D_Color32(74,54,37,255)
                    :C2D_Color32(44,43,40,255),0.48f);
                (void)renderer.draw_menu_art(1,box.x(),box.y(),
                    box.width(),box.height(),0.75f);
                if(selected){
                    const auto gold=C2D_Color32(216,181,98,255);
                    renderer.draw_solid_rect(box.x(),box.y(),
                        box.width(),2.0f,gold,0.84f);
                    renderer.draw_solid_rect(box.x(),box.y()+box.height()-2.0f,
                        box.width(),2.0f,gold,0.84f);
                }
                if(!enabled)
                    renderer.draw_solid_rect(box.x(),box.y(),
                        box.width(),box.height(),
                        C2D_Color32(13,17,17,160),0.85f);
            };
            const auto back=[&]()noexcept {
                wood(minidayz::menu::kSubpageBack,false,true);
                renderer.draw_text("B / TOUCH : BACK",91.0f,219.0f,0.36f);
            };
            using minidayz::menu::BootPage;
            const auto render_source_list=[&](
                const minidayz::menu::SourceMenuRecord* records,
                std::size_t total,const char* title) noexcept {
                renderer.draw_text(title,88.0f,10.0f,0.47f);
                if(menu_list_index_>=total)return;
                if(menu_detail_open_){
                    // Every original description can now be read in full
                    // instead of the previous two-line truncation.
                    const auto& record=records[menu_list_index_];
                    char heading[54]{};
                    std::snprintf(heading,sizeof(heading),"%.42s",record.name);
                    renderer.draw_text(heading,17.0f,34.0f,0.35f);
                    const char* cursor=record.description;
                    std::size_t wrapped_line=0;
                    int displayed=0;
                    while(*cursor && wrapped_line<menu_detail_scroll_+8){
                        while(*cursor==' '||*cursor=='\n'||*cursor=='\r')
                            ++cursor;
                        if(!*cursor)break;
                        char line[46]{};
                        std::size_t count=0,break_at=0;
                        while(cursor[count] && cursor[count]!='\n' &&
                              cursor[count]!='\r' && count<43){
                            if(cursor[count]==' ')break_at=count;
                            ++count;
                        }
                        if(count==43 && cursor[count] && break_at>0)
                            count=break_at;
                        if(count==0){++cursor;continue;}
                        std::memcpy(line,cursor,count);
                        line[count]=0;
                        if(wrapped_line>=menu_detail_scroll_ && displayed<8){
                            renderer.draw_text(line,14.0f,
                                61.0f+displayed*17.0f,0.28f);
                            ++displayed;
                        }
                        cursor+=count;
                        ++wrapped_line;
                    }
                    renderer.draw_text("UP/DOWN : SCROLL   A : LIST",
                        47.0f,199.0f,0.25f);
                    return;
                }
                const std::size_t offset=(menu_list_index_/4)*4;
                for(std::size_t row=0;row<4;++row){
                    const auto index=offset+row;
                    if(index>=total)break;
                    const minidayz::menu::TouchBounds rect{
                        15,static_cast<int>(43+row*30),
                        305,static_cast<int>(71+row*30)};
                    wood(rect,index==menu_list_index_,true);
                    char name[65]{};
                    std::snprintf(name,sizeof(name),"%.45s",records[index].name);
                    renderer.draw_text(name,25.0f,rect.y()+6.0f,0.31f);
                }
                renderer.draw_text("A / TOUCH : VIEW REQUIREMENTS",
                    39.0f,180.0f,0.25f);
                char page[32]{};
                std::snprintf(page,sizeof(page),"%u / %u  L/R PAGE",
                    static_cast<unsigned>(menu_list_index_+1),
                    static_cast<unsigned>(total));
                renderer.draw_text(page,99.0f,199.0f,0.24f);
            };
            switch(menu_page_){
            case BootPage::main:{
                renderer.draw_text("MINI DAYZ",116.0f,8.0f,0.49f);
                static constexpr float label_x[5]={
                    125.0f,125.0f,99.0f,134.0f,137.0f};
                for(std::size_t i=0;i<5;++i){
                    const bool available=i!=0||has_saved_game_;
                    wood(minidayz::menu::kMenuRows[i],
                        menu_selection_==static_cast<minidayz::menu::MenuEntry>(i),
                        available);
                    renderer.draw_text(
                        i==0&&!has_saved_game_?"NO SAVE":
                            minidayz::menu::kMainLabels[i],
                        i==0&&!has_saved_game_?132.0f:label_x[i],
                        minidayz::menu::kMenuRows[i].y()+8.0f,0.39f);
                }
                wood(minidayz::menu::kCharacterButton,false,true);
                wood(minidayz::menu::kMultiplayerButton,false,true);
                renderer.draw_text("X  CHARACTER",31.0f,218.0f,0.31f);
                renderer.draw_text("Y  MULTIPLAYER",178.0f,218.0f,0.27f);
                break;
            }
            case BootPage::character:{
                renderer.draw_text("CHOOSE SURVIVOR",75.0f,13.0f,0.58f);
                renderer.draw_solid_rect(39.0f,46.0f,242.0f,91.0f,
                    C2D_Color32(35,37,31,225),0.31f);
                char info[96]{};
                std::snprintf(info,sizeof(info),"SURVIVOR %u / %u",
                    static_cast<unsigned>(selected_skin_+1),
                    static_cast<unsigned>(minidayz::stage1::kOriginalPlayerSkinCount));
                renderer.draw_text(info,101.0f,53.0f,0.36f);
                // Show the actual ORIGINAL survivor art on the character
                // menu, rather than a text-only preview or fake silhouette.
                // The same source skin selection is used in the world renderer.
                const auto preview_skin=
                    minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object;
                if(!renderer.draw_source_animation(preview_skin,"idle_down",0,
                        160.0f,104.0f,1.20f,1.20f,0.59f) &&
                   preview_skin!=minidayz::kSourcePlayerObject)
                    (void)renderer.draw_source_animation(
                        minidayz::kSourcePlayerObject,"idle_down",0,
                        160.0f,104.0f,1.20f,1.20f,0.59f);
                wood(minidayz::menu::kSkinPrevious,false,true);
                wood(minidayz::menu::kSkinNext,false,true);
                renderer.draw_text("< PREVIOUS",32.0f,163.0f,0.33f);
                renderer.draw_text("NEXT >",211.0f,163.0f,0.33f);
                back();
                break;
            }
            case BootPage::difficulty:{
                renderer.draw_text("SELECT DIFFICULTY",72.0f,18.0f,0.45f);
                for(std::size_t i=0;i<3;++i){
                    wood(minidayz::menu::kDifficultyRows[i],
                        difficulty_index_==i,true);
                    renderer.draw_text(minidayz::menu::kDifficultyLabels[i],
                        i==1?125.0f:133.0f,
                        minidayz::menu::kDifficultyRows[i].y()+9.0f,0.41f);
                }
                renderer.draw_text("A / TOUCH : START NEW GAME",
                    46.0f,194.0f,0.28f);
                back();
                break;
            }
            case BootPage::achievements:
                render_source_list(minidayz::menu::kSourceAchievements,
                    minidayz::menu::kSourceAchievementCount,"ACHIEVEMENTS");
                back();
                break;
            case BootPage::unlocks:
                if(unlock_weapons_page_)
                    render_source_list(minidayz::menu::kSourceWeaponUnlocks,
                        minidayz::menu::kSourceWeaponUnlockCount,"WEAPON UNLOCKS");
                else
                    render_source_list(minidayz::menu::kSourceUnlocks,
                        minidayz::menu::kSourceUnlockCount,"SURVIVOR UNLOCKS");
                if(!menu_detail_open_)
                    renderer.draw_text("X : SWITCH UNLOCK CATEGORY",
                        49.0f,163.0f,0.24f);
                back();
                break;
            case BootPage::options:{
                renderer.draw_text("OPTIONS",111.0f,15.0f,0.50f);
                const minidayz::menu::TouchBounds audio_row{64,70,256,117};
                wood(audio_row,false,true);
                renderer.draw_text(stage2_audio_.muted()?"SOUND : OFF":"SOUND : ON",
                    103.0f,87.0f,0.41f);
                renderer.draw_text("A / TOUCH TO CHANGE",62.0f,129.0f,0.31f);
                // Original UI localization contains Stick position and
                // Language. On 3DS the Circle Pad is physical and the
                // existing single-player translations are English only.
                renderer.draw_text("STICK : 3DS CIRCLE PAD",58.0f,152.0f,0.28f);
                renderer.draw_text("LANGUAGE : ENGLISH",75.0f,173.0f,0.28f);
                renderer.draw_text("SELECT IN GAME : FPS",69.0f,192.0f,0.26f);
                back();
                break;
            }
            case BootPage::multiplayer:
                renderer.draw_text("MULTIPLAYER",95.0f,21.0f,0.48f);
                renderer.draw_text("MP NETWORK GAMEPLAY",55.0f,91.0f,0.31f);
                renderer.draw_text("NOT YET PORTED TO 3DS",52.0f,117.0f,0.31f);
                back();
                break;
            }
            if(generation_failed_){
                renderer.draw_solid_rect(6.0f,3.0f,308.0f,25.0f,
                    C2D_Color32(86,27,21,245),0.89f);
                renderer.draw_text("WORLD LOAD FAILED - CHOOSE NEW GAME",
                    18.0f,8.0f,0.25f);
            }
            return;
        }
        if (paused_) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255),0.01f);
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.05f);
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
                                     C2D_Color32(18, 22, 21, 255),0.01f);
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.05f);
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
        if(map_open_) {
            render_source_map(renderer);
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
        // Original inventory backdrop: 419x283. Preserve its aspect ratio
        // as 320x216, rather than deforming it to the 320x240 display.
        if (!ui_art(minidayz::stage1::kOriginalInventoryBackgroundArt,
                    0.0f, 12.0f, 320.0f, 216.0f, 0.05f)) {
            // The fallback must be BEHIND every original inventory sprite,
            // not at the default .5f depth where it hides its own artwork.
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(70, 60, 48, 255),0.01f);
        }
        renderer.draw_solid_rect(6.0f, 6.0f, 308.0f, 25.0f, C2D_Color32(54, 68, 55, 255));
        renderer.draw_solid_rect(258.0f,1.0f,56.0f,13.0f,
            C2D_Color32(92,81,52,255),0.68f);
        renderer.draw_text("Y MAP",267.0f,3.0f,0.27f);
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
        {
            std::uint16_t selected_id=0;
            if(selected_inventory_slot_>=0){
                const auto* selected=inventory_.slot(
                    static_cast<std::size_t>(selected_inventory_slot_));
                if(selected && !selected->empty())
                    selected_id=selected->item_id;
            }else if(selected_equipment_slot_>=0){
                const int slot=selected_equipment_slot_;
                selected_id=slot==5 ? inventory_.equipped_backpack()
                    : slot==6 ? inventory_.equipped_satchel()
                    : slot==8 ? equipment_.quick_pistol()
                    : equipment_.equipped(
                        gear_slot_for_card(slot));
            }
            if(selected_id!=0){
                if(const char* name=minidayz::items::source_item_name(selected_id))
                    renderer.draw_text(name,10.0f,25.0f,0.19f);
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
                "HELM", "COAT", "VEST", "PANTS", "HAND", "PACK", "SATCH", "GUN", "QUICK"
            };
            for (std::size_t i = 0; i < minidayz::items::ui::kEquipment.size(); ++i) {
                // Actual original frame dimensions, not stretched equal cards:
                // coat/vest 91x62, backpack/satchel 216x62, inventory 57x54.
                const auto& card=minidayz::items::ui::kEquipment[i];
                const float x=static_cast<float>(card.x);
                const float y=static_cast<float>(card.y);
                const float w=static_cast<float>(card.w);
                const float h=static_cast<float>(card.h);
                const auto& art=
                    i==1 ? minidayz::stage1::kOriginalEquipmentCoatArt
                    : i==2 ? minidayz::stage1::kOriginalEquipmentVestArt
                    : i==5 ? minidayz::stage1::kOriginalEquipmentBackpackArt
                    : i==6 ? minidayz::stage1::kOriginalEquipmentSatchelArt
                    : minidayz::stage1::kOriginalInventoryCardArt;
                // The original held weapon is in its own slot. The source
                // inventory-card image is square, so never stretch it 56x34.
                if(i==8){
                    // gui_slot is an ITEM ICON ATLAS: its first frame is a
                    // soda can, NOT an empty slot. Use the genuine original
                    // inventory card image for the separate pistol cell.
                    if(!ui_art(minidayz::stage1::kOriginalInventoryCardArt,
                               x,y,w,h,0.24f))
                        renderer.draw_solid_rect(x,y,w,h,
                            C2D_Color32(54,46,35,255),0.24f);
                }else if(i==7){
                    // gui_firearm is NOT a card background: Construct
                    // uses its 32 varying-size frames for actual guns.
                    // The slot remains source parchment while the exact
                    // selected weapon icon is rotated in foreground below.
                    if(!ui_art(minidayz::stage1::kOriginalInventoryCardArt,
                               x,y,w,h,0.24f))
                        renderer.draw_solid_rect(x,y,w,h,
                            C2D_Color32(146,130,99,255),0.24f);
                }else if(i==4 || i==3){
                    renderer.draw_solid_rect(x,y,w,h,
                        C2D_Color32(131,118,91,255),0.24f);
                    if(i==4)
                        (void)ui_art(minidayz::stage1::kOriginalInventoryCardArt,
                                     x+11.0f,y+2.0f,30.0f,30.0f,0.27f);
                    else
                        (void)ui_art(art,x+12.0f,y+1.0f,32.0f,32.0f,0.27f);
                }else if(i==5 || i==6){
                    // Source bags are 216x62. Fit within the smaller 3DS
                    // toolbar cell WITHOUT stretching their native aspect.
                    if(!ui_art(art,x,y+2.0f,w,w*62.0f/216.0f,0.24f))
                        renderer.draw_solid_rect(x,y,w,h,
                            C2D_Color32(146,130,99,255),0.24f);
                }else if(!ui_art(art,x,y,w,h,0.24f)){
                    renderer.draw_solid_rect(x,y,w,h,
                        C2D_Color32(146,130,99,255),0.24f);
                }
                if(drag_equipped_source_slot_==static_cast<int>(i) ||
                   selected_equipment_slot_==static_cast<int>(i)){
                    const auto gold=C2D_Color32(229,185,83,255);
                    renderer.draw_solid_rect(x,y,w,2.0f,gold,0.77f);
                    renderer.draw_solid_rect(x,y+h-2.0f,w,2.0f,gold,0.77f);
                }
                const auto item_id=
                    i==5 ? inventory_.equipped_backpack()
                    : i==6 ? inventory_.equipped_satchel()
                    : i==8 ? equipment_.quick_pistol()
                    : equipment_.equipped(
                        gear_slot_for_card(static_cast<int>(i)));
                const auto condition=
                    i==5 ? inventory_.equipped_backpack_condition()
                    : i==6 ? inventory_.equipped_satchel_condition()
                    : i==8 ? equipment_.quick_pistol_snapshot().condition
                    : equipment_.snapshot().conditions[gear_index_for_card(static_cast<int>(i))];
                const float icon_size=i==1||i==2?34.0f:
                    i==0||i==3?31.0f:i==7?42.0f:i==8?24.0f:26.0f;
                const float icon_x=x+(i==7?w*0.5f:i==8?14.0f:
                    i==1||i==2?18.0f:i==0||i==3?w*0.5f:16.0f);
                const float icon_y=y+(i==7?h*0.5f:i==8?16.0f:
                    i==1||i==2?30.0f:i==0?25.0f:i==3?16.0f:17.0f);
                if(item_id!=0){
                    if(i==7)
                        (void)renderer.draw_item_icon(item_id,icon_x,icon_y,
                            42.0f,57.0f,0.86f,1.5707963268f);
                    else
                        (void)renderer.draw_item_icon(item_id,
                            icon_x,icon_y,icon_size,icon_size,0.86f);
                    const auto fraction=i==8
                        ? equipment_.quick_pistol_snapshot().fractional_loss
                        : (i<5 || i==7)
                            ? equipment_.fraction_snapshot().fractional_loss[
                                gear_index_for_card(static_cast<int>(i))]:0u;
                    const float exact_condition=static_cast<float>(condition)-
                        static_cast<float>(fraction)/100.0f;
                    char durability[16]{};
                    std::snprintf(durability,sizeof(durability),"%.0f%%",
                        static_cast<double>(exact_condition));
                    const bool long_coat=i==1||i==2;
                    if(long_coat){
                        // In the original inventory the jacket and vest
                        // show condition and separate armor/warmth lines.
                        renderer.draw_text(durability,x+45.0f,y+9.0f,0.25f);
                        minidayz::items::SourceItemProtection effects{};
                        if(minidayz::items::source_item_protection(
                            item_id,effects)){
                            const float armor=i==1
                                ? effects.outerwear:effects.vest;
                            if(effects.warmth!=0.0f && i==1){
                                char heat[24]{};
                                std::snprintf(heat,sizeof(heat),"HEAT %+.0f",
                                    static_cast<double>(effects.warmth));
                                renderer.draw_text(heat,x+43.0f,y+26.0f,0.22f);
                            }
                            if(armor!=0.0f){
                                char armor_text[24]{};
                                std::snprintf(armor_text,sizeof(armor_text),
                                    "ARMOR %+.0f",static_cast<double>(armor));
                                renderer.draw_text(armor_text,x+43.0f,y+41.0f,0.22f);
                            }
                        }
                    } else if(i==7){
                        renderer.draw_text(durability,x+8.0f,y+h-14.0f,0.22f);
                    } else if(i==8){
                        renderer.draw_text("Q",x+31.0f,y+3.0f,0.20f);
                        renderer.draw_text(durability,x+29.0f,y+19.0f,0.15f);
                    } else if(i==0||i==3) {
                        renderer.draw_text(durability,x+10.0f,
                            y+(i==3?23.0f:41.0f),0.21f);
                    } else {
                        const char* name=minidayz::items::source_item_name(item_id);
                        renderer.draw_text(name&&*name?name:equipment_labels[i],
                                           x+34.0f,y+7.0f,0.18f);
                        renderer.draw_text(durability,x+34.0f,y+20.0f,0.22f);
                    }
                }else{
                    const auto& empty_icon=
                        i==0 ? minidayz::stage1::kOriginalHelmetPlaceholderArt
                        : i==1 ? minidayz::stage1::kOriginalOuterwearPlaceholderArt
                        : i==2 ? minidayz::stage1::kOriginalVestPlaceholderArt
                        : i==3 ? minidayz::stage1::kOriginalPantsPlaceholderArt
                        : i==5 ? minidayz::stage1::kOriginalBackpackPlaceholderArt
                        : minidayz::stage1::kOriginalInventoryCardArt;
                    // Empty melee, firearm and quick slots must not render
                    // the soda can first frame from gui_slot's icon atlas.
                    if(i!=4 && i!=7 && i!=8 && empty_icon.object!=65535)
                        (void)ui_art(empty_icon,icon_x-icon_size*0.5f,
                            icon_y-icon_size*0.5f,icon_size,icon_size,0.82f);
                    if(i!=0&&i!=3)
                        renderer.draw_text(equipment_labels[i],
                            i==7?x+9.0f:i==8?x+25.0f:
                                x+(i==1||i==2?43.0f:34.0f),
                            y+8.0f,i==8?0.17f:0.20f);
                }
            }
            static constexpr const char* actions[] = {
                "USE", "ALT", "DROP", "PICK", "SPLIT"
            };
            for (int i = 0; i < 5; ++i) {
                const float x = static_cast<float>(i * 64);
                renderer.draw_solid_rect(x+1.0f,
                    static_cast<float>(minidayz::items::ui::kActions.y),
                    62.0f,13.0f,C2D_Color32(90,76,55,255),0.61f);
                renderer.draw_text(actions[i],x+13.0f,
                    static_cast<float>(minidayz::items::ui::kActions.y+3),0.27f);
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
                            // The source gui_slot-sheet0.png contains ICONS
                            // (soda, tools, weapons), not an empty-slot frame.
                            // Paint an empty wooden cell; render a real item
                            // sprite ONLY if the storage contains an item.
                            renderer.draw_solid_rect(x,y,28.0f,28.0f,
                                C2D_Color32(37,31,26,220),0.30f);
                            if (!item || item->empty()) continue;
                            (void)renderer.draw_item_icon(item->item_id,
                                x + 14.0f, y + 15.0f, 24.0f, 24.0f);
                            char amount[12]{};
                            std::snprintf(amount, sizeof(amount), "%u",
                                          static_cast<unsigned>(item->count));
                            if(item->count>1)
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
            // Show original item descriptions for both backpack items
            // and equipped gear; tapping worn clothing selects it rather
            // than silently removing it.
            const auto* selected = selected_inventory_slot_ >= 0
                ? inventory_.slot(static_cast<std::size_t>(selected_inventory_slot_))
                : nullptr;
            std::uint16_t item_id=selected && !selected->empty()
                ?selected->item_id:0;
            unsigned count=selected && !selected->empty()?selected->count:1;
            unsigned exact_hundredths=selected && !selected->empty()
                ?static_cast<unsigned>(selected->condition)*100u-
                 static_cast<unsigned>(selected->fractional_loss):10000u;
            if(!item_id && selected_equipment_slot_>=0){
                const int slot=selected_equipment_slot_;
                item_id=slot==5 ? inventory_.equipped_backpack()
                    :slot==6 ? inventory_.equipped_satchel()
                    :slot==8 ? equipment_.quick_pistol()
                    :equipment_.equipped(
                        gear_slot_for_card(slot));
                const auto condition=slot==5
                    ?inventory_.equipped_backpack_condition()
                    :slot==6 ? inventory_.equipped_satchel_condition()
                    :slot==8 ? equipment_.quick_pistol_snapshot().condition
                    :equipment_.snapshot().conditions[gear_index_for_card(slot)];
                const auto fraction=slot==8
                    ?equipment_.quick_pistol_snapshot().fractional_loss
                    :slot<5 || slot==7
                        ?equipment_.fraction_snapshot().fractional_loss[
                            gear_index_for_card(slot)]
                        :0u;
                exact_hundredths=static_cast<unsigned>(condition)*100u-
                    static_cast<unsigned>(fraction);
            }
            if (item_id) {
                const char* description =
                    minidayz::items::source_item_description(item_id);
                char heading[80]{};
                std::snprintf(heading, sizeof(heading),
                    "ITEM %u x%u CONDITION %u.%02u%%",
                    static_cast<unsigned>(item_id),
                    count,
                    exact_hundredths / 100u, exact_hundredths % 100u);
                renderer.draw_text(heading, 16.0f, 101.0f, 0.27f);
                if (description && *description) {
                    // Simple bounded word-wrapping for the original XML text.
                    // Do not substitute guessed descriptions for missing rows.
                    const char* pos = description;
                    unsigned content_lines=0;
                    for (std::size_t line = 0; line < 64 && *pos; ++line) {
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
                        if(line>=details_scroll_ && line<details_scroll_+6)
                            renderer.draw_text(buffer,16.0f,
                                125.0f+static_cast<float>(line-details_scroll_)*14.0f,0.24f);
                        ++content_lines;
                        pos = next;
                        while (*pos == ' ' || *pos == '\n') ++pos;
                    }
                    if(details_scroll_>=content_lines)details_scroll_=0;
                } else {
                    renderer.draw_text("NO ORIGINAL DESCRIPTION IN ROOT XML",
                                       16.0f, 134.0f, 0.23f);
                }
            } else {
                renderer.draw_text("SELECT AN ITEM, THEN OPEN DETAILS",
                                   16.0f, 130.0f, 0.26f);
            }
            renderer.draw_text("TAP TEXT TO SCROLL - TAP NAME TO CLOSE",
                               14.0f, 205.0f, 0.18f);
            renderer.draw_solid_rect(10.0f, 225.0f, 300.0f, 13.0f,
                                     C2D_Color32(54, 68, 55, 255));
            renderer.draw_text("ORIGINAL ITEM DESCRIPTION", 35.0f, 226.0f, 0.24f);
            renderer.draw_text("CRAFT", 132.0f, 226.0f, 0.29f);
            return;
        }
        // Original GUI 'gui_slot' is a 30x30 PIXEL square. Keep that
        // aspect ratio instead of deforming it into the previous 56x29
        // cyan-looking grid. A 3DS-compatible 8x2 arrangement gives 15
        // actual inventory cells per page (one unused space at the end).
        const int page_start=static_cast<int>(inventory_page_*kInventoryPageSize);
        for(int local=0;local<minidayz::items::ui::kPageSize;++local) {
            const int index=page_start+local;
            if(index<0 || static_cast<std::size_t>(index)>=inventory_.capacity())
                continue;
            const int row=local/minidayz::items::ui::kColumns;
            const int col=local%minidayz::items::ui::kColumns;
            const float x=static_cast<float>(
                minidayz::items::ui::kSlotX+
                col*minidayz::items::ui::kSlotStepX);
            const float y=static_cast<float>(
                minidayz::items::ui::kSlotY+
                row*minidayz::items::ui::kSlotStepY);
            const float size=static_cast<float>(minidayz::items::ui::kSlotSize);
            const bool selected=index==selected_inventory_slot_;
            const bool restricted=inventory_.is_exclusive_satchel_slot(
                static_cast<std::size_t>(index));
            // The original gui_inventory background already has wooden
            // slot dividers. gui_slot-sheet0.png is an ITEM ICON ATLAS,
            // whose first frame is a can. Painting that frame in EVERY
            // empty slot caused the sixteen identical cans in Manic Emu.
            // Keep the original panel visible, draw only actual owned items.
            renderer.draw_solid_rect(x,y,size,size,
                C2D_Color32(31,28,25,70),0.30f);
            if(selected || drag_target_slot_==index) {
                const u32 gold=C2D_Color32(229,190,96,255);
                renderer.draw_solid_rect(x,y,size,2.0f,gold,0.92f);
                renderer.draw_solid_rect(x,y+size-2.0f,size,2.0f,gold,0.92f);
                renderer.draw_solid_rect(x,y,2.0f,size,gold,0.92f);
                renderer.draw_solid_rect(x+size-2.0f,y,2.0f,size,gold,0.92f);
            }
            if(restricted){
                const auto satchel=inventory_.equipped_satchel();
                renderer.draw_text(satchel==459||satchel==753?"ARMY":"MED",
                                   x+3.0f,y+2.0f,0.14f);
            }
            const auto* item=inventory_.slot(static_cast<std::size_t>(index));
            if(!item||item->empty())continue;
            // Item-ID to source Construct object comes from MDZItemTable.
            (void)renderer.draw_item_icon(item->item_id,
                x+size*0.5f,y+size*0.5f,27.0f,27.0f,0.86f);
            if(item->count>1){
                char amount[10]{};
                std::snprintf(amount,sizeof(amount),"%u",
                    static_cast<unsigned>(item->count));
                renderer.draw_text(amount,x+16.0f,y+21.0f,0.20f);
            }
            const float condition=static_cast<float>(item->condition)-
                static_cast<float>(item->fractional_loss)/100.0f;
            if(condition>0.0f && condition<100.0f){
                renderer.draw_solid_rect(x+2.0f,y+size-3.0f,
                    size-4.0f,2.0f,C2D_Color32(31,33,27,255),0.94f);
                renderer.draw_solid_rect(x+2.0f,y+size-3.0f,
                    (size-4.0f)*condition/100.0f,2.0f,
                    C2D_Color32(199,165,86,255),0.95f);
            }
        }
        // Source item sprite follows the stylus during an actual drag,
        // while the underlying original parchment slots remain visible.
        // Draw only on the lower touchscreen; world player art on TOP
        // must never be redrawn as a UI drag ghost.
        if(drag_moved_ && (drag_source_slot_>=0 ||
                           drag_equipped_source_slot_>=0)){
            std::uint16_t moving_id=0;
            if(drag_source_slot_>=0){
                const auto* moving=inventory_.slot(
                    static_cast<std::size_t>(drag_source_slot_));
                if(moving && !moving->empty())moving_id=moving->item_id;
            }else{
                const int ui_slot=drag_equipped_source_slot_;
                moving_id=ui_slot==5?inventory_.equipped_backpack()
                    :ui_slot==6?inventory_.equipped_satchel()
                    :ui_slot==8?equipment_.quick_pistol()
                    :equipment_.equipped(gear_slot_for_card(ui_slot));
            }
            if(moving_id!=0)
                (void)renderer.draw_item_icon(moving_id,
                    static_cast<float>(drag_touch_x_),
                    static_cast<float>(drag_touch_y_),
                    27.0f,27.0f,0.98f);
        }
        const std::size_t page_count =
            (inventory_.capacity() + kInventoryPageSize - 1) / kInventoryPageSize;
        char page_label[48]{};
        std::snprintf(page_label, sizeof(page_label), "<  BACKPACK  %u/%u  >",
            static_cast<unsigned>(inventory_page_ + 1),
            static_cast<unsigned>(page_count));
        renderer.draw_solid_rect(10.0f,225.0f,300.0f,13.0f,
                                 C2D_Color32(74,66,50,255),0.61f);
        renderer.draw_text("PAGE <",14.0f,226.0f,0.25f);
        renderer.draw_text("CRAFT",132.0f,226.0f,0.30f);
        renderer.draw_text(page_label,213.0f,226.0f,0.23f);
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
                                     C2D_Color32(24, 34, 29, 255),0.01f);
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 400.0f, 240.0f, 0.05f);
            if (!renderer.draw_menu_art(0, 42.0f, 50.0f, 316.0f, 80.0f))
                renderer.draw_text("MINI DAYZ", 76.0f, 72.0f, 1.05f);
            renderer.draw_text("GENERATING NEW SURVIVAL WORLD", 51.0f, 198.0f, 0.37f);
            return;
        }
        if (menu_active_) {
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f, 0.0f, 400.0f, 240.0f,
                                     C2D_Color32(24, 34, 29, 255),0.01f);
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 400.0f, 240.0f, 0.05f);
            if (!renderer.draw_menu_art(0, 42.0f, 50.0f, 316.0f, 80.0f))
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
                                     C2D_Color32(24, 20, 19, 255),0.01f);
            if (!renderer.draw_source_sprite_fitted(
                    minidayz::stage1::kOriginalDeathScreenArt.object,
                    minidayz::stage1::kOriginalDeathScreenArt.animation,
                    minidayz::stage1::kOriginalDeathScreenArt.frame,
                    0.0f, 0.0f, 400.0f, 240.0f, 0.05f))
                (void)renderer.draw_menu_art(
                    2, 0.0f, 0.0f, 400.0f, 240.0f, 0.05f);
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
                                     C2D_Color32(24, 34, 29, 255),0.01f);
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 400.0f, 240.0f, 0.05f);
            if (!renderer.draw_menu_art(0, 42.0f, 35.0f, 316.0f, 80.0f))
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
            visibility_index_.query(camera_x,camera_y,view_w,view_h,
                                    visible_world_placements_);
            for (const auto i : visible_world_placements_) {
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
                // Manic Emu confirms larger Z is in front. Keep source
                // layer 31 (background buildings) behind player .57 and
                // source layers 41-43 (foreground buildings/trees) above.
                const float layer_depth = std::clamp(
                    0.15f + 0.0125f * static_cast<float>(placement->layer),
                    0.15f, 0.80f);
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
                                          tent->x, tent->y, 28.0f, 28.0f, 0.54f);
        }

        for (std::size_t i = 0; i < stashes_.count(); ++i) {
            const auto* stash = stashes_.stash(i);
            if (!stash || stash->x < camera_x - 24.0f ||
                stash->x > camera_x + view_w + 24.0f ||
                stash->y < camera_y - 24.0f ||
                stash->y > camera_y + view_h + 24.0f) continue;
            (void)renderer.draw_item_icon(
                minidayz::items::kOriginalSuppliesStashItemId,
                stash->x, stash->y, 28.0f, 28.0f, 0.54f);
        }

        for (std::size_t i = 0; i < ground_loot_.count(); ++i) {
            const auto* loot = ground_loot_.entry(i);
            if (!loot || loot->x < camera_x - 24.0f ||
                loot->x > camera_x + view_w + 24.0f ||
                loot->y < camera_y - 24.0f ||
                loot->y > camera_y + view_h + 24.0f) continue;
            (void)renderer.draw_item_icon(loot->item.item_id,
                loot->x, loot->y, 18.0f, 18.0f, 0.54f);
        }

        // Keep original pixel-art proportions. The previous arbitrary 0.55x
        // shrink made the survivor and zombies tiny in the 3DS viewport.
        if (npc_ready_) {
            for (std::size_t i = 0; i < npcs_.count(); ++i) {
                const auto* npc = npcs_.actor(i);
                if (!npc || !npc->alive ||
                    npc->x < camera_x - 48.0f || npc->x > camera_x + view_w + 48.0f ||
                    npc->y < camera_y - 48.0f || npc->y > camera_y + view_h + 48.0f)
                    continue;
                if (!renderer.draw_source_sprite(npc->source_object,
                        npc->animation_index, npc->animation_frame,
                        npc->x, npc->y, 1.0f, 1.0f))
                    renderer.draw_solid_rect(npc->x - 5.0f, npc->y - 5.0f,
                        10.0f, 12.0f, C2D_Color32(130, 145, 78, 255));
                // The original NPC is composed from a skin sprite and a
                // separate eyes sprite pinned to the same position.
                (void)renderer.draw_source_sprite(npc->eyes_object, 0, 0,
                    npc->x, npc->y, 1.0f, 1.0f);
            }
        }

        const char* skin_direction="down";
        switch(p.facing){
        case minidayz::PlayerFacing::down: skin_direction="down"; break;
        case minidayz::PlayerFacing::up: skin_direction="up"; break;
        case minidayz::PlayerFacing::left: skin_direction="left"; break;
        case minidayz::PlayerFacing::right: skin_direction="right"; break;
        }
        char skin_animation[32]{};
        std::snprintf(skin_animation,sizeof(skin_animation),"%s_%s",
            p.animation==minidayz::PlayerAnimation::idle?"idle":"run",
            skin_direction);
        const auto source_skin_object=
            minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object;
        // An armed survivor uses the ORIGINAL player's held-weapon stance
        // when available, not an ordinary empty-hands idle pose. This works
        // for all 20 skins and chooses only authentic Construct animations.
        const auto held_item=(active_quick_pistol_ && equipment_.quick_pistol()!=0)
            ? equipment_.quick_pistol()
            : equipment_.equipped(active_held_slot_)!=0
                ?equipment_.equipped(active_held_slot_)
                : equipment_.equipped(minidayz::items::EquipmentSlot::weapon)!=0
                    ?equipment_.equipped(minidayz::items::EquipmentSlot::weapon)
                    : equipment_.equipped(minidayz::items::EquipmentSlot::hands);
        const auto* held_visual=minidayz::stage1::original_equipment_visual(
            held_item);
        const auto* weapon_pose=minidayz::stage1::original_equipment_pose(
            held_visual,p.animation!=minidayz::PlayerAnimation::idle,true);
        bool sprite_drawn=false;
        if(p.alive && weapon_pose &&
            std::strcmp(weapon_pose,"idle")!=0 &&
            std::strcmp(weapon_pose,"run")!=0) {
            char armed_animation[32]{};
            std::snprintf(armed_animation,sizeof(armed_animation),"%s_%s",
                          weapon_pose,skin_direction);
            sprite_drawn=renderer.draw_source_animation(source_skin_object,
                armed_animation,player_.source_animation_frame(),
                p.x,p.y,1.0f,1.0f,0.57f);
        }
        if(p.alive && !sprite_drawn)
            sprite_drawn=renderer.draw_source_animation(source_skin_object,
                skin_animation,player_.source_animation_frame(),
                p.x,p.y,1.0f,1.0f,0.57f);
        // If an optional survivor texture is missing from RomFS, use the
        // game's ORIGINAL default survivor as a last-resort sprite. This
        // keeps the player visible without substituting invented artwork;
        // equipment overlays remain source-derived. Never replace the
        // selected skin when its original animation loaded successfully.
        if(p.alive && !sprite_drawn &&
           source_skin_object!=minidayz::kSourcePlayerObject)
            sprite_drawn=renderer.draw_source_animation(
                minidayz::kSourcePlayerObject,skin_animation,
                player_.source_animation_frame(),
                p.x,p.y,1.0f,1.0f,0.57f);
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
                    player_.source_animation_frame(),p.x,p.y,1.0f,1.0f,depth);
            };
            // Source-authored equipment back-to-front. The originals use
            // separate layers on top of the chosen character base skin.
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::pants),false,0.575f);
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::outerwear),false,0.580f);
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::vest),false,0.585f);
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::head),false,0.590f);
            draw_equipped_layer(inventory_.equipped_backpack(),false,0.595f);
            // Original satchels are separate wearable storage equipment,
            // not part of the main backpack slot. Show both when equipped.
            draw_equipped_layer(inventory_.equipped_satchel(),false,0.600f);
            // Draw only the selected held firearm OR melee overlay, not
            // both weapons through the same hand at the same time.
            draw_equipped_layer(held_item,true,0.605f);
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

    static constexpr std::size_t kInventoryPageSize = minidayz::items::ui::kPageSize;
    static constexpr std::size_t kCraftPageSize = 3;

    static int equipment_slot_at(int x, int y) noexcept {
        return minidayz::items::ui::equipment_at(x,y);
    }
    // UI cards 0-4 match the native armor/melee equipment indices.
    // Card 7 is the original dedicated firearm category, whose legacy
    // EquipmentSnapshot index is 6. Cards 5/6 are native bags, not armor.
    static constexpr minidayz::items::EquipmentSlot gear_slot_for_card(int ui) noexcept {
        return ui==7 ? minidayz::items::EquipmentSlot::weapon
                     : static_cast<minidayz::items::EquipmentSlot>(ui);
    }
    static constexpr std::size_t gear_index_for_card(int ui) noexcept {
        return static_cast<std::size_t>(gear_slot_for_card(ui));
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
        return minidayz::items::ui::inventory_at(x,y,page);
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
        details_scroll_=0;
        tap_was_selected_ = false;
        selected_inventory_slot_ = -1;
        selected_equipment_slot_=-1;
        active_held_slot_=minidayz::items::EquipmentSlot::hands;
        active_quick_pistol_=false;
        drag_moved_=false;
        drag_source_slot_=-1;
        drag_equipped_source_slot_=-1;
        drag_target_slot_=-1;
        drag_target_equipment_slot_=-1;
        save_elapsed_ = 0.0f;
        new_game_generation_ = true;
        death_saved_ = false;
        paused_ = false;
        // Keep the old Continue entry available if the new world fails.
        // The save file is replaced only after successful generation.
        begin_world_generation();
    }

    // Original Construct Game_events "Start events":
    // PlayerBase SetPosition UID 9846853259707545: (600,
    // 1500 + random(LayoutHeight-5000)) on Map.
    // CurrentLevel==4 overrides with UID 641409757696071:
    // (random(1000,16500),373).
    // Never retry, snap to a road, or shift the position on collision:
    // those extra rules changed the original starting position.
    bool place_new_player_on_generated_map() noexcept {
        const auto& bounds=world_.bounds();
        if(!(bounds.width>0.0f && bounds.height>0.0f))return false;
        std::mt19937 rng(world_seed_ ^ 0x73506177u);
        const bool snow=world_level_==
            minidayz::stage1::kSnowPlayerStartLevel;
        const float range=std::max(0.0f,bounds.height-
            minidayz::stage1::kNormalPlayerStartLayoutMargin);
        std::uniform_real_distribution<float> random_normal_y(0.0f,range);
        std::uniform_real_distribution<float> random_snow_x(
            minidayz::stage1::kSnowPlayerStartXMin,
            minidayz::stage1::kSnowPlayerStartXMax);
        const float px=snow?random_snow_x(rng):
            minidayz::stage1::kNormalPlayerStartX;
        const float py=snow?minidayz::stage1::kSnowPlayerStartY:
            minidayz::stage1::kNormalPlayerStartYBase+random_normal_y(rng);
        if(!std::isfinite(px)||!std::isfinite(py))return false;
        auto state=travel_generation_?player_.state():make_initial_state();
        state.x=px;
        state.y=py;
        player_=minidayz::PlayerSystem(state);
        player_.set_source_skin(
            minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
        return true;
    }

    void begin_world_generation() noexcept {
        generation_failed_ = false;
        world_ready_ = false;
        npc_ready_ = false;
        menu_active_ = true;
        loading_world_ = world_.begin_source_generation(world_seed_, "Map", world_level_);
        if (!loading_world_) {
            generation_failed_ = true;
            menu_page_ = minidayz::menu::BootPage::main;
            menu_selection_ = minidayz::menu::default_selection(has_saved_game_);
            boot_input_gate_.reset();
        }
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
        for (const auto i : scene->collision_index_.candidates(x,y)) {
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
    minidayz::terrain::SourceObstacleIndex collision_index_{};
    minidayz::terrain::VisibleSourcePlacementIndex visibility_index_{};
    std::vector<std::uint32_t> visible_world_placements_{};
    minidayz::NpcSystem npcs_{};
    std::uint32_t world_seed_{};
    std::uint8_t world_level_{};
    bool travel_generation_{};
    float save_elapsed_{};
    float footstep_elapsed_{};
    bool world_ready_{};
    bool npc_ready_{};
    bool menu_active_{true};
    minidayz::menu::StartInputGate boot_input_gate_{};
    minidayz::menu::BootPage menu_page_{minidayz::menu::BootPage::main};
    minidayz::menu::MenuEntry menu_selection_{minidayz::menu::MenuEntry::new_game};
    minidayz::SurvivalDifficulty selected_difficulty_{minidayz::SurvivalDifficulty::regular};
    std::size_t difficulty_index_{1};
    std::size_t menu_list_index_{};
    bool unlock_weapons_page_{};
    bool menu_detail_open_{};
    std::size_t menu_detail_scroll_{};
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
    int drag_equipped_source_slot_{-1};
    int selected_equipment_slot_{-1};
    minidayz::items::EquipmentSlot active_held_slot_{
        minidayz::items::EquipmentSlot::hands};
    bool active_quick_pistol_{};
    int drag_start_x_{};
    int drag_start_y_{};
    int drag_touch_x_{};
    int drag_touch_y_{};
    bool drag_moved_{};
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
    std::size_t details_scroll_{};
    bool map_open_{};
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
