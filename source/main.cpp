#include "minidayz/engine.hpp"
#include "minidayz/source_special_terrain.hpp"
#include "minidayz/boot_menu.hpp"
#include "minidayz/menu_preferences.hpp"
#include "minidayz/generated/menu_source_catalog.hpp"
#include "minidayz/player.hpp"
#include "minidayz/world.hpp"
#include "minidayz/world_visual_save.hpp"
#include "minidayz/source_door_auto.hpp"
#include "minidayz/source_door_solid.hpp"
#include "minidayz/source_building_polygon_collision.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/source_furniture_interaction.hpp"
#include "minidayz/source_interior_container.hpp"
#include "minidayz/source_world_layer_depth.hpp"
#include "minidayz/source_mapgen_geometry.hpp"
#include "minidayz/generated/ground_floor_source.hpp"
#include "minidayz/source_world_sprite_visibility.hpp"
#include "minidayz/generated/source_roof_foundation_animations.hpp"
#include "minidayz/source_world_proximity.hpp"
#include "minidayz/source_map_overlay.hpp"
#include "minidayz/source_minimap_frames.hpp"
#include "minidayz/terrain_collision.hpp"
#include "minidayz/terrain_obstacle_index.hpp"
#include "minidayz/source_tree_visibility_collision.hpp"
#include "minidayz/terrain_visible_index.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include "minidayz/generated/source_building_frame_geometry.hpp"
#include "minidayz/terrain_source_visibility.hpp"
#include "minidayz/terrain_roads.hpp"
#include "minidayz/generated/sprite_catalog.hpp"
#include "minidayz/generated/player_skin_source.hpp"
#include "minidayz/generated/player_hands_source.hpp"
#include "minidayz/generated/source_dead_skin_art.hpp"
#include "minidayz/generated/player_spawn_source.hpp"
#include "minidayz/player_equipment_visual.hpp"
#include "minidayz/source_equipment_animation.hpp"
#include "minidayz/source_player_layers.hpp"
#include "minidayz/source_player_altmove.hpp"
#include "minidayz/source_energy_drink_boost.hpp"
#include "minidayz/source_player_base_geometry.hpp"
#include "minidayz/npc_system.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/inventory.hpp"
#include "minidayz/inventory_touch_layout.hpp"
#include "minidayz/equipment.hpp"
#include "minidayz/item_use.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/original_car_trunk.hpp"
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
#include "minidayz/original_initial_building_loot.hpp"
#include "minidayz/generated/inventory_ui_art.hpp"
#include "minidayz/generated/item_catalog.hpp"

#include <algorithm>
#include <limits>
#include <cmath>
#include <random>
#include <vector>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <memory>
#include <new>
#include <utility>
#include <sys/stat.h>

namespace {
void boot_checkpoint(const char* stage, bool reset=false) noexcept;
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
          legacy_world_generation_(restored && restored->version < 23u),
          legacy_split_roads_(restored && restored->version < 24u),
          legacy_crash_coordinates_(restored && restored->version < 25u),
          legacy_site_generation_(restored && restored->version < 26u),
          legacy_start_order_(restored && restored->version < 27u),
          legacy_category3_events_(restored && restored->version < 28u),
          legacy_category5_events_(restored && restored->version < 29u),
          legacy_category46_events_(restored && restored->version < 30u),
          legacy_category7_events_(restored && restored->version < 31u),
          legacy_exact_crash_selection_(restored && restored->version < 32u),
          legacy_active_location_events_(restored && restored->version < 33u),
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
        if (restored && !source_energy_drink_.restore(restored->source_energy_seconds))
            source_energy_drink_.clear();
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
        preview_rules_.collision_half_width =
            minidayz::stage1::kSourcePlayerBaseHalfWidth;
        preview_rules_.collision_half_height =
            minidayz::stage1::kSourcePlayerBaseHalfHeight;
        preview_rules_.collision_offset_x =
            minidayz::stage1::kSourcePlayerBaseCollisionOffsetX;
        preview_rules_.collision_offset_y =
            minidayz::stage1::kSourcePlayerBaseCollisionOffsetY;
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
        legacy_world_generation_=restored->version < 23u;
        legacy_split_roads_=restored->version < 24u;
        legacy_crash_coordinates_=restored->version < 25u;
        legacy_site_generation_=restored->version < 26u;
        legacy_start_order_=restored->version < 27u;
        legacy_category3_events_=restored->version < 28u;
        legacy_category5_events_=restored->version < 29u;
        legacy_category46_events_=restored->version < 30u;
        legacy_category7_events_=restored->version < 31u;
        legacy_exact_crash_selection_=restored->version < 32u;
        legacy_active_location_events_=restored->version < 33u;
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
        twilight_fade_.reset();
        if(!rain_timer_.restore(restored->source_rain))return false;
        if(!loot_timer_.restore(restored->source_loot))return false;
        if(!item_use_runtime_.restore(restored->pending_item_use))return false;
        if(!source_energy_drink_.restore(restored->source_energy_seconds))return false;
        if(!perks_.restore(restored->perks))return false;
        pending_craft_recipe_=restored->pending_craft.recipe_index;
        pending_craft_seconds_=restored->pending_craft.seconds_remaining;
        crafting_feedback_=pending_craft_recipe_<minidayz::items::kSourceCraftRecipeCount
            ?CraftFeedback::pending:CraftFeedback::none;
        continue_world_visual_state_=true;
        new_game_generation_=false;
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

    bool use_original_inventory_item(std::size_t slot,bool secondary) noexcept {
        const auto* item=inventory_.slot(slot);
        if(!item || item->empty())return false;
        const auto item_id=item->item_id;
        const bool used=minidayz::items::use_item_from_inventory(
            inventory_,slot,player_,preview_rules_,secondary,
            perks_.item_use_flags(),&item_use_runtime_);
        // Game_events item_activate(63) is the original Edrink boost;
        // a failed item activation must never create a free speed buff.
        if(source_energy_drink_.on_successful_item_use(
                item_id,secondary,used))
            stage2_audio_.play_original_world_sound(
                minidayz::OriginalWorldSound::drinkSoda0);
        return used;
    }

    bool write_survivor_save(std::uint32_t seed, std::uint8_t level,
                             const minidayz::PlayerState& survivor,
                             const minidayz::items::GroundLootSystem& loot,
                             const minidayz::items::TentStorageSystem& tents,
                             const minidayz::items::StashStorageSystem* stashes, bool legacy_world_generation,
                              bool legacy_split_roads = false,
                              bool legacy_crash_coordinates = false,
                              bool legacy_site_generation = false,
                              bool legacy_start_order = false,
                              bool legacy_category3_events = false,
                              bool legacy_category5_events = false,
                              bool legacy_category46_events = false,
                              bool legacy_category7_events = false,
                              bool legacy_exact_crash_selection = false,
                              bool legacy_active_location_events = false) noexcept {
        minidayz::persistence::PendingCraftSnapshot craft{};
        if (pending_craft_recipe_ < minidayz::items::kSourceCraftRecipeCount &&
            pending_craft_seconds_ > 0.0f) {
            craft.recipe_index = static_cast<std::uint16_t>(pending_craft_recipe_);
            craft.seconds_remaining = pending_craft_seconds_;
        }
        return write_saved_game(seed, survivor, inventory_, equipment_, loot,
            tents, &item_use_runtime_, &perks_, stashes, &day_night_,
            &rain_timer_, &loot_timer_, &craft, level, &source_energy_drink_, legacy_world_generation, legacy_split_roads, legacy_crash_coordinates, legacy_site_generation, legacy_start_order, legacy_category3_events, legacy_category5_events, legacy_category46_events, legacy_category7_events, legacy_exact_crash_selection, legacy_active_location_events);
    }

    bool write_scene_save() noexcept {
        if (!write_survivor_save(world_seed_, world_level_, player_.state(),
                                 ground_loot_, tents_, &stashes_, legacy_world_generation_, legacy_split_roads_, legacy_crash_coordinates_, legacy_site_generation_, legacy_start_order_, legacy_category3_events_, legacy_category5_events_, legacy_category46_events_, legacy_category7_events_, legacy_exact_crash_selection_, legacy_active_location_events_)) return false;
        // Persist changed original object frames separately, with the same
        // seed/level and a checksum. Do not report Save success unless
        // world state is recorded too.
        return minidayz::persistence::write_world_visual_state(
            world_,world_level_,"sdmc:/3ds/Minidayz/world_visual.sav",
            "sdmc:/3ds/Minidayz/world_visual.tmp");
    }

    void on_enter() noexcept override {
        // The source tilemap_water and tilemap_forest sprites carry Solid
        // behavior. Load the exact Map RLE occupancy once from the RomFS
        // so the player collides with visible tiles, not huge empty tilemap
        // bounding boxes. Failure leaves basic sprite collision enabled.
        boot_checkpoint("loading terrain collision");
        (void)terrain_collision_.load(
            "romfs:/gfx/original_map_details_layout.bin");
        boot_checkpoint("terrain collision loaded");
        // Audio is optional. RomFS is already mounted by Engine::initialize;
        // DSP failure must never prevent inventory or crafting gameplay.
        boot_checkpoint("initializing audio");
        (void)stage2_audio_.initialize();
        boot_checkpoint("audio initialization returned");
        // Original Menu_Events starts ambient on entering the main menu.
        // Playback uses the same day-loop channel as gameplay, at -10 dB.
        stage2_audio_.start_menu_ambience();
        boot_checkpoint("menu ambience returned");
        std::mt19937 menu_rng(world_seed_);
        (void)minidayz::world_source::build_special_terrain(
            minidayz::world_source::SourceSpecialTerrainEvent::MenuStart,
            menu_rng,menu_source_terrain_);
        boot_checkpoint("menu terrain ready");
    }

    void on_exit() noexcept override {
        if (world_ready_ && !menu_active_)
            (void)write_scene_save();
        stage2_audio_.shutdown();
    }

    void update(const minidayz::InputState& input, float dt) noexcept override {
        if (loading_world_) {
            auto* destination = travel_.destination();
            const bool traveling = destination != nullptr;
            if (!(traveling ? travel_.advance() : world_.advance_source_generation())) {
                fail_world_generation(traveling);
                return;
            }
            auto& generating = traveling ? *destination : world_;
            if (generating.generation_complete()) {
                // Original Game_events calls Trigger_spawn for actual
                // Sprite218 markers during create_map_element. Reproduce
                // that INITIAL event once for a newly generated Map, before
                // writing the first save. Continue restores existing ground
                // loot and must never re-roll already looted buildings.
                // Keep large ground-drop storage off the 3DS's narrow
                // update() stack; dense Map levels may carry 300+ source
                // marker placements even though not every marker drops.
                std::unique_ptr<minidayz::items::GroundLootSystem>
                    generated_initial_loot{};
                if(new_game_generation_ || traveling){
                    generated_initial_loot.reset(new(std::nothrow)
                        minidayz::items::GroundLootSystem{});
                    if(!generated_initial_loot){
                        fail_world_generation(traveling);
                        return;
                    }
                    if(!generating.source_active_location_events()){
                        const auto result=minidayz::items::source_spawn_initial_building_loot(
                            generating,*generated_initial_loot,generating.source_random());
                        if(!result.complete()){
                            fail_world_generation(traveling);
                            return;
                        }
                    }
                }
                if (new_game_generation_ || traveling) {
                    auto survivor = traveling ? player_.state() : make_initial_state();
                    if (!place_new_player_on_generated_map(generating, survivor)) {
                        fail_world_generation(traveling);
                        return;
                    }
                    if(generating.source_active_location_events() &&
                       (!generating.start_source_location_reload({survivor.x,survivor.y}) ||
                        !generating.execute_source_location_creation(*generated_initial_loot))){
                        fail_world_generation(traveling);
                        return;
                    }
                    if (traveling) {
                        const minidayz::items::TentStorageSystem empty_tents{};
                        if (!write_survivor_save(generating.seed(), generating.generation_level(),
                                survivor, *generated_initial_loot, empty_tents, nullptr, false)) {
                            fail_world_generation(true);
                            return;
                        }
                        has_saved_game_ = true;
                    }
                    player_ = minidayz::PlayerSystem(survivor);
                    player_.set_source_skin(
                        minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
                }
                if (traveling) {
                    world_seed_ = generating.seed();
                    world_level_ = generating.generation_level();
                    legacy_world_generation_ = false;
                    legacy_split_roads_ = false;
                    legacy_crash_coordinates_ = false;
                    legacy_site_generation_ = false;
                    legacy_start_order_ = false;
                    legacy_category3_events_ = false;
                    legacy_category5_events_ = false;
                    legacy_category46_events_ = false;
                    legacy_category7_events_ = false;
                    legacy_exact_crash_selection_ = false;
                    legacy_active_location_events_ = false;
                    world_ = std::move(generating);
                    travel_.cancel();
                    ground_loot_=std::move(*generated_initial_loot);
                    tents_.clear();
                    stashes_.clear();
                    selected_inventory_slot_ = -1;
                    storage_open_ = false;
                    crafting_open_ = false;
                    perks_open_ = false;
                    details_open_ = false;
                    paused_ = false;
                    stage2_audio_.stop_environment_ambience();
                    stage2_audio_.stop_rain_loop();
                    footstep_elapsed_ = 0.0f;
                    save_elapsed_ = 0.0f;
                }
                if(new_game_generation_ && !traveling)
                    ground_loot_=std::move(*generated_initial_loot);
                loading_world_ = false;
                world_ready_ = true;
                if (continue_world_visual_state_) {
                    // Legacy saves may have no sidecar. A missing/stale file
                    // never bypasses original generation or alters the seed.
                    const bool visual_ok=minidayz::persistence::read_world_visual_state(
                        world_,world_level_,
                        "sdmc:/3ds/Minidayz/world_visual.sav");
                    continue_world_visual_state_=false;
                    if(!visual_ok && world_.source_active_location_events()){
                        fail_world_generation(false);
                        return;
                    }
                }
                source_location_creation_retry_=false;
                if(!(new_game_generation_ || traveling) || !world_.source_active_location_events()){
                    (void)world_.start_source_location_reload({player_.state().x,player_.state().y});
                    if(world_.source_active_location_events()){
                        source_location_creation_retry_=!world_.execute_source_location_creation(ground_loot_);
                        source_location_creation_retry_count_=ground_loot_.count();
                    }
                }
                collision_index_.rebuild(world_);
                visibility_index_.rebuild(world_);
                rebuild_source_automatic_doors();
                rebuild_source_house_roofs();
                rebuild_source_furniture();
                rebuild_source_interior_containers();
                npc_ready_ = npcs_.initialize(world_);
                new_game_generation_ = false;
                menu_active_ = false;
                stage2_audio_.set_environment_ambience(
                    !day_night_.day_mode(),world_level_==4);
                if (rain_timer_.active()) stage2_audio_.start_rain_loop(world_level_==4);
                else stage2_audio_.stop_rain_loop();
                // Only overwrite the existing save after a new world succeeds.
                if (write_scene_save()) has_saved_game_ = true;
            }
            return;
        }
        if (menu_active_) {
            // Restore original menu ambience after returning from gameplay.
            stage2_audio_.start_menu_ambience();
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
                        preview_rules_.collision_half_width=minidayz::stage1::kSourcePlayerBaseHalfWidth;
                        preview_rules_.collision_half_height=minidayz::stage1::kSourcePlayerBaseHalfHeight;
                        preview_rules_.collision_offset_x=minidayz::stage1::kSourcePlayerBaseCollisionOffsetX;
                        preview_rules_.collision_offset_y=minidayz::stage1::kSourcePlayerBaseCollisionOffsetY;
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
                    stage2_audio_.poll_ambience_playback();
                    // Independent short original pickup sound test: distinguishes
                    // a quiet ambient loop from muted emulator CSND output.
                    if(input.pressed & KEY_X)
                        stage2_audio_.play_item_pickup();
                    if((input.pressed & KEY_A) ||
                        (tap && input.touch.px>=64 && input.touch.px<=256 &&
                                input.touch.py>=70 && input.touch.py<=117)){
                        stage2_audio_.set_muted(!stage2_audio_.muted());
                        persist_menu_settings();
                        if(!stage2_audio_.muted() && menu_active_){
                            stage2_audio_.start_menu_ambience();
                        }else if(!stage2_audio_.muted() && world_ready_){
                            stage2_audio_.set_environment_ambience(
                                !day_night_.day_mode(),world_level_==4);
                            if(rain_timer_.active())
                                stage2_audio_.start_rain_loop(world_level_==4);
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
            // Root dead-survivor Sprite813 has six frames at 1 fps,
            // nonlooping, on Map/items_on_ground18. Advance the original
            // visual clock while the 3DS death screen is showing.
            source_dead_animation_elapsed_=std::min(6.0f,
                source_dead_animation_elapsed_+std::max(0.0f,dt));
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
        if(!source_bed_build_.active &&
           ((input.pressed & KEY_Y) || map_button_tap)) {
            map_open_=!map_open_;
            if(map_button_tap)touch_down_=true;
        }
        if(map_open_) {
            const bool tap=input.touching && !touch_down_;
            touch_down_=input.touching;
            if(tap && input.touch.py>=32 && input.touch.py<202) {
                // The 3DS touch map uses original Array POI codes and
                // exact Mapgen's transposed world positions. Identify a
                // touched section by its source-defined type, not by an
                // invented minimap icon or guessed building density.
                const auto& grid=world_.grid();
                const auto& bounds=world_.bounds();
                const minidayz::map_overlay::MapRect viewport{
                    12.0f,32.0f,296.0f,170.0f};
                float best_distance=11.0f*11.0f;
                auto selected=minidayz::map_overlay::CellKind::ground;
                for(int y=0;y<grid.height();++y)
                    for(int x=0;x<grid.width();++x){
                        const auto* marker=world_.map_marker(
                            static_cast<std::size_t>(y)*grid.width()+x);
                        if(!marker)continue;
                        const auto kind=minidayz::map_overlay::marker_cell_kind(*marker);
                        if(!minidayz::map_overlay::is_location(kind))continue;
                        const auto rect=minidayz::map_overlay::source_cell_rect(
                            x,y,bounds,viewport);
                        const float dx=input.touch.px-(rect.x+rect.width*0.5f);
                        const float dy=input.touch.py-(rect.y+rect.height*0.5f);
                        const float sq=dx*dx+dy*dy;
                        if(sq<best_distance){
                            best_distance=sq;
                            selected=kind;
                        }
                    }
                map_selected_kind_=selected;
            }
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
                (void)use_original_inventory_item(
                    selected_index,(input.pressed & KEY_X) != 0);
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
                (minidayz::items::kSourceCraftRecipeCount + 2 + kCraftPageSize - 1) /
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
            if (recipe >= minidayz::items::kSourceCraftRecipeCount &&
                recipe < minidayz::items::kSourceCraftRecipeCount+2) {
                const auto variant=static_cast<std::uint8_t>(
                    recipe-minidayz::items::kSourceCraftRecipeCount);
                crafting_feedback_=begin_source_bed_craft(variant)
                    ? CraftFeedback::none : CraftFeedback::missing_or_full;
                crafting_feedback_elapsed_=0.0f;
            } else if (source_recipe &&
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
                            (void)use_original_inventory_item(selected,
                                action == 1);
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
                        (void)use_original_inventory_item(source,false);
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
        const auto before_twilight=day_night_.snapshot();
        (void)day_night_.update(std::max(0.0f, dt));
        twilight_fade_.advance(before_twilight,day_night_.snapshot(),std::max(0.0f,dt));
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
                    (world_.source_active_location_events() &&
                     (placement->source_initial_loot_consumed || !world_.source_placement_active(*placement))) ||
                    !world_.bounds().contains({placement->x, placement->y}))
                    continue;
                std::uint16_t category = 0;
                // Editor-authored source markers keep their original UID.
                // Runtime-created marker 218 has no editor UID; the
                // original location event sets its instance variable 0.
                // Use that actual source category instead of dropping every
                // generated marker (which disabled loot across most towns).
                if (!minidayz::items::source_world_loot_category(
                        placement->object, placement->source_uid,
                        placement->source_loot_category, category))
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
            if (rain_timer_.active()) stage2_audio_.start_rain_loop(world_level_==4);
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
        // Construct Doors_auto is an overlap event; it must be allowed to
        // open a source-frame-0 doorway before the next player collision
        // sweep. Otherwise movement blocks at the door and the player can
        // never cross its "in" imagepoint. Each source door may change
        // frame at most once per update: opened and closed frames can
        // have DIFFERENT imagepoints. Never trigger both sounds in one tick.
        std::fill(source_door_changed_this_tick_.begin(),
                  source_door_changed_this_tick_.end(),0u);
        // Original open/close events both execute System Wait(0.01)
        // before changing door 349 frame. Complete those waits once per
        // game tick; original WAV dispatch follows the committed change.
        advance_source_door_waits(dt);
        advance_source_furniture_wait(dt);
        advance_source_sleep_wait(dt);
        advance_source_bed_dismantle(dt);
        advance_source_bed_build(input,dt);
        // Native A adapts original mobile marker506 interactions, without
        // inventing full Construct touchscreen pathfinding. A selected item
        // retains its own independent inventory-use action above.
        if ((input.pressed & KEY_A) && selected_inventory_slot_ < 0 &&
            !source_bed_build_.active && !source_pump_pending_.runtime_uid &&
            !request_source_manual_door() &&
            !request_source_car_trunk())
            request_source_furniture();
        if ((input.pressed & KEY_X) && selected_inventory_slot_ < 0 &&
            !source_bed_build_.active)
            request_source_bed_dismantle();
        update_source_automatic_doors();
        if(source_bed_build_.active || source_pump_pending_.runtime_uid)movement={};
        // Source PlayerBase193 AltMove MaxSpeed UID156884066447061.
        // Apply only the verified runtime modifiers currently provided by
        // their original event subsystems. Remaining original damage,
        // terrain and equipment modifiers are not silently fabricated.
        minidayz::stage1::SourcePlayerAltMoveModifiers speed_effects{};
        speed_effects.runner_bonus=static_cast<float>(
            perks_.source_runner_speed_bonus());
        // Original Player stats condition UID189013257553857 is
        // perk_regen > 1, not perk_regen > 0. The bonus activates only
        // on the second purchased Regen level (PlayerBase.var5=10).
        speed_effects.regen_bonus=minidayz::stage1::source_player_regen_speed_bonus(
            perks_.level(minidayz::items::SourcePerk::regen));
        // Root PlayerBase variable 4 is +15 only while the original
        // 60-second Edrink timer is active (source UID435941792839578).
        source_energy_drink_.update(dt);
        speed_effects.energy_boost=source_energy_drink_.bonus();
        const float source_speed=
            minidayz::stage1::source_player_altmove_max_speed(speed_effects);
        preview_rules_.walk_speed=source_speed;
        preview_rules_.run_speed=source_speed;
        player_.update_movement(movement, dt, preview_rules_,
                                &WorldPreviewScene::blocked, this);
        const bool locations_changed=world_.update_source_location_reload(
            {player_.state().x,player_.state().y},dt);
        if(world_.source_active_location_events() &&
           (locations_changed || (source_location_creation_retry_ &&
             ground_loot_.count()<source_location_creation_retry_count_))){
            source_location_creation_retry_=!world_.execute_source_location_creation(ground_loot_);
            source_location_creation_retry_count_=ground_loot_.count();
        }
        // Re-evaluate after movement for source-frame-1 closing transitions
        // and newly reached door triggers. The frame guard prevents double
        // sounds when both checks observe the same state.
        update_source_automatic_doors();
        update_source_house_roofs(dt);
        update_source_furniture_bed_frame();
        update_source_interior_containers();
        source_furniture_feedback_seconds_=
            std::max(0.0f,source_furniture_feedback_seconds_-std::max(0.0f,dt));
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
                play_source_footsteps(player_.state().x,player_.state().y);
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
        // 3DS-ONLY CUSTOM MAP PRESENTATION, expressly limited to the lower
        // screen. No custom art, geometry, or spawning is used in the WORLD.
        // Read original Array518 roads and Sprite514 marker codes directly;
        // reuse the same original Mapgen coordinates as the world renderer.
        renderer.reset_camera();
        const auto& grid=world_.grid();
        const auto& bounds=world_.bounds();
        renderer.draw_solid_rect(0.0f,0.0f,320.0f,240.0f,
            C2D_Color32(18,25,21,255),0.01f);
        renderer.draw_solid_rect(4.0f,3.0f,312.0f,23.0f,
            C2D_Color32(43,60,51,255),0.10f);
        renderer.draw_text("WORLD MAP",111.0f,5.0f,0.47f);
        char map_seed[24]{};
        std::snprintf(map_seed,sizeof(map_seed),"SEED %08X",
            static_cast<unsigned>(world_.seed()));
        renderer.draw_text(map_seed,10.0f,9.0f,0.19f);
        char map_level[16]{};
        std::snprintf(map_level,sizeof(map_level),"LVL %u",
            static_cast<unsigned>(world_.generation_level()+1u));
        renderer.draw_text(map_level,264.0f,9.0f,0.21f);
        if(grid.width()==0||grid.height()==0||
           !(bounds.width>0.0f&&bounds.height>0.0f)){
            renderer.draw_text("MAP NOT GENERATED",63.0f,105.0f,0.33f);
            return;
        }

        // The touch coordinate rectangle is intentionally identical to
        // update()'s source_cell_rect selection rectangle.
        const minidayz::map_overlay::MapRect area{
            12.0f,32.0f,296.0f,170.0f};
        renderer.draw_solid_rect(area.x-3.0f,area.y-3.0f,
            area.width+6.0f,area.height+6.0f,
            C2D_Color32(89,101,73,255),0.13f);
        renderer.draw_solid_rect(area.x,area.y,area.width,area.height,
            C2D_Color32(46,67,49,255),0.17f);

        // Clean geographic grid; 33 original Array rows are east-west,
        // 16 Array columns are north-south in the Construct world.
        // Pixel-snap 3DS map-only line artwork to avoid subpixel shimmer.
        for(int y=0;y<grid.height();++y)
            for(int x=0;x<grid.width();++x){
                const auto* marker=world_.map_marker(
                    static_cast<std::size_t>(y)*grid.width()+x);
                if(!marker||
                   !minidayz::map_overlay::original_minimap_marker_is_visible(
                       *marker))continue;
                const auto cell=minidayz::map_overlay::source_cell_rect(
                    x,y,bounds,area);
                if(cell.width<=0.0f||cell.height<=0.0f||
                   cell.x+cell.width<area.x||cell.y+cell.height<area.y||
                   cell.x>area.x+area.width||
                   cell.y>area.y+area.height)continue;
                // Extremely subtle boundaries keep the map readable without
                // repeating 20x20 source minimap atlas art at only 8px.
                renderer.draw_solid_rect(std::floor(cell.x),
                    std::floor(cell.y),std::max(1.0f,cell.width),
                    0.45f,C2D_Color32(63,82,58,255),0.19f);
                renderer.draw_solid_rect(std::floor(cell.x),
                    std::floor(cell.y),0.45f,
                    std::max(1.0f,cell.height),
                    C2D_Color32(63,82,58,255),0.19f);

                // The actual Array518 road codes determine corridors.
                // Array-X roads are world-vertical, Array-Y roads world-
                // horizontal. Markers 30/31/50 etc may override Sprite514
                // but do not erase their underlying Array518 road cell.
                const auto road=grid.at(x,y);
                const bool vert=minidayz::terrain::horizontal_road(road);
                const bool hori=minidayz::terrain::vertical_road(road);
                const float cx=cell.x+cell.width*0.5f;
                const float cy=cell.y+cell.height*0.5f;
                if(vert){
                    renderer.draw_solid_rect(std::floor(cx-2.2f),
                        std::floor(cell.y),4.5f,std::ceil(cell.height)+1.0f,
                        C2D_Color32(30,39,35,255),0.33f);
                    renderer.draw_solid_rect(std::floor(cx-1.1f),
                        std::floor(cell.y),2.4f,std::ceil(cell.height)+1.0f,
                        C2D_Color32(177,161,116,255),0.35f);
                }
                if(hori){
                    renderer.draw_solid_rect(std::floor(cell.x),
                        std::floor(cy-2.2f),std::ceil(cell.width)+1.0f,4.5f,
                        C2D_Color32(30,39,35,255),0.33f);
                    renderer.draw_solid_rect(std::floor(cell.x),
                        std::floor(cy-1.1f),std::ceil(cell.width)+1.0f,2.4f,
                        C2D_Color32(177,161,116,255),0.35f);
                }
            }

        // Source marker types label actual places. The only invented part
        // is the *map display styling*, never world objects or location data.
        for(int y=0;y<grid.height();++y)
            for(int x=0;x<grid.width();++x){
                const auto* marker=world_.map_marker(
                    static_cast<std::size_t>(y)*grid.width()+x);
                if(!marker||
                   !minidayz::map_overlay::original_minimap_marker_is_visible(
                       *marker))continue;
                const auto kind=minidayz::map_overlay::marker_cell_kind(*marker);
                if(!minidayz::map_overlay::is_location(kind))continue;
                const auto cell=minidayz::map_overlay::source_cell_rect(
                    x,y,bounds,area);
                const float cx=std::floor(cell.x+cell.width*0.5f);
                const float cy=std::floor(cell.y+cell.height*0.5f);
                if(cx<area.x||cy<area.y||
                   cx>area.x+area.width||cy>area.y+area.height)continue;
                u32 icon_color=C2D_Color32(217,200,139,255);
                switch(kind){
                case minidayz::map_overlay::CellKind::military:
                    icon_color=C2D_Color32(99,169,192,255);break;
                case minidayz::map_overlay::CellKind::city:
                    icon_color=C2D_Color32(236,171,105,255);break;
                case minidayz::map_overlay::CellKind::hospital:
                    icon_color=C2D_Color32(238,234,211,255);break;
                case minidayz::map_overlay::CellKind::firestation:
                    icon_color=C2D_Color32(217,109,93,255);break;
                case minidayz::map_overlay::CellKind::village:
                    icon_color=C2D_Color32(222,200,130,255);break;
                default:
                    icon_color=C2D_Color32(137,203,149,255);break;
                }
                renderer.draw_solid_rect(cx-3.0f,cy-3.0f,7.0f,7.0f,
                    C2D_Color32(13,25,23,255),0.49f);
                renderer.draw_solid_rect(cx-2.0f,cy-2.0f,5.0f,5.0f,
                    icon_color,0.51f);
            }

        // Distinct marker: red source player world coordinates. On-map
        // crosshair is intentionally custom because only the map may use
        // non-original presentation.
        const auto position=minidayz::map_overlay::player_point(
            player_.state().x,player_.state().y,bounds,area);
        const float mx=std::floor(position.x),my=std::floor(position.y);
        renderer.draw_solid_rect(mx-4.0f,my-4.0f,9.0f,9.0f,
            C2D_Color32(11,20,18,255),0.72f);
        renderer.draw_solid_rect(mx-3.0f,my-1.0f,7.0f,3.0f,
            C2D_Color32(242,103,75,255),0.77f);
        renderer.draw_solid_rect(mx-1.0f,my-3.0f,3.0f,7.0f,
            C2D_Color32(242,103,75,255),0.78f);

        const auto* selected_name=minidayz::map_overlay::location_name(
            map_selected_kind_);
        if(selected_name){
            char label[48]{};
            std::snprintf(label,sizeof(label),"SECTION: %s",selected_name);
            renderer.draw_text(label,12.0f,208.0f,0.29f);
        }else{
            renderer.draw_text("TOUCH A MARKER TO IDENTIFY THE LOCATION",
                11.0f,208.0f,0.24f);
        }
        renderer.draw_solid_rect(8.0f,225.0f,304.0f,14.0f,
            C2D_Color32(77,72,51,255),0.67f);
        renderer.draw_text("Y / B / TOUCH: BACK TO INVENTORY",
            21.0f,226.0f,0.25f);
    }

    void render_bottom(minidayz::Renderer& renderer) noexcept override {
        if (loading_world_) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255),0.01f);
            (void)renderer.draw_menu_art(2, 0.0f, 0.0f, 320.0f, 240.0f, 0.05f);
            renderer.draw_text("GENERATING WORLD", 45.0f, 52.0f, 0.52f);
            static constexpr const char* steps[] = {
                "GENERATING GRID AND ROADS", "BUILDING MAP MARKERS",
                "GENERATING LOCATION PROPOSALS", "CRASHES AND BUNKERS",
                "VEHICLES AND LOCATION LOADING", "FINALIZING WORLD"
            };
            const auto* destination = travel_.destination();
            const auto phase = destination ? destination->generation_phase()
                                           : world_.generation_phase();
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
                renderer.draw_text(stage2_audio_.ambience_status(),
                    40.0f,195.0f,0.23f);
                renderer.draw_text("A : SOUND    X : TEST",59.0f,129.0f,0.31f);
                // Original UI localization contains Stick position and
                // Language. On 3DS the Circle Pad is physical and the
                // existing single-player translations are English only.
                renderer.draw_text("STICK : 3DS CIRCLE PAD",58.0f,152.0f,0.28f);
                renderer.draw_text("LANGUAGE : ENGLISH",75.0f,173.0f,0.28f);
                // Audio status now owns this line; no overlapping FPS label.
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
                      "T %.1f  B %.1f  I %.1f",
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
                (minidayz::items::kSourceCraftRecipeCount + 2 + kCraftPageSize - 1) /
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
                if (!recipe && i>=minidayz::items::kSourceCraftRecipeCount &&
                    i<minidayz::items::kSourceCraftRecipeCount+2) {
                    const auto variant=static_cast<std::uint8_t>(
                        i-minidayz::items::kSourceCraftRecipeCount);
                    const float y=99.0f+static_cast<float>(row)*40.0f;
                    const bool ready=minidayz::world_source::
                        source_bed_materials_available(inventory_,variant);
                    renderer.draw_solid_rect(12.0f,y,296.0f,36.0f,
                        ready?C2D_Color32(65,95,64,255)
                             :C2D_Color32(49,56,49,255));
                    // Source object951 frame0, original 37x21 bed image,
                    // never a guessed inventory icon or a whole PNG sheet.
                    (void)renderer.draw_source_sprite(951,0,0,
                        27.0f,y+17.0f,0.65f,0.65f,0.54f);
                    renderer.draw_text(variant?"BUILD BED (BEAR HIDE)":
                        "BUILD BED (DEERSKIN)",43.0f,y+2.0f,0.24f);
                    renderer.draw_text(ready?"READY":"NEED",
                        263.0f,y+2.0f,0.19f);
                    char ingredients[100]{};
                    std::snprintf(ingredients,sizeof(ingredients),
                        "WOOD 50: %u/1 + HIDE %u: %u/1",
                        static_cast<unsigned>(inventory_.count(50)),
                        static_cast<unsigned>(
                            minidayz::world_source::source_bed_hides(variant)),
                        static_cast<unsigned>(inventory_.count(
                            minidayz::world_source::source_bed_hides(variant))));
                    renderer.draw_text(ingredients,43.0f,y+19.0f,0.18f);
                    renderer.draw_text("PLACE",265.0f,y+26.0f,0.16f);
                    continue;
                }
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
        if(!crafting_open_ && !storage_open_ && !perks_open_ &&
           source_furniture_feedback_seconds_>0.0f &&
           source_furniture_feedback_ && *source_furniture_feedback_){
            renderer.draw_solid_rect(12.0f,140.0f,296.0f,23.0f,
                                     C2D_Color32(39,63,46,230));
            renderer.draw_text(source_furniture_feedback_,
                               16.0f,144.0f,0.29f);
        }
        if(near_source_bridge_exit()){
            renderer.draw_solid_rect(18.0f, 104.0f, 284.0f, 21.0f,
                                     C2D_Color32(39, 63, 46, 230));
            renderer.draw_text(travel_failed_ ? "TRAVEL FAILED. A: RETRY" : "A: TRAVEL TO NEXT MAP",
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
            // Original Menu_Events/Animateds paints Tilemap230 once,
            // not once per frame or into the survival world's RNG stream.
            (void)renderer.draw_source_special_terrain(
                menu_source_terrain_,0.0f,0.0f,400.0f,240.0f,0.06f);
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
        // Menu textures otherwise remain allocated after switching from
        // boot/loading screens. Recover their VRAM before the first
        // generated forest, village and house atlas loads. Pausing or
        // dying lazily reloads the exact original menu images when needed.
        renderer.release_menu_sheets();
        const auto& p = player_.state();
        constexpr float view_w = 400.0f;
        constexpr float view_h = 240.0f;
        // Original 60px source tile quads must land on integer 3DS pixels.
        // Subpixel camera translation made adjacent tile borders shimmer
        // as the survivor walked; world physics and saved XY stay exact.
        const float camera_x = std::floor(std::clamp(
            p.x - view_w * 0.5f, 0.0f,
            std::max(0.0f, world_.bounds().width - view_w)));
        const float camera_y = std::floor(std::clamp(
            p.y - view_h * 0.5f, 0.0f,
            std::max(0.0f, world_.bounds().height - view_h)));
        renderer.set_camera(camera_x, camera_y);
        // The source Map uses Construct Tilemaps, not sprites. The native
        // floor must be drawn from the original ground_lvl1 tile sheet,
        // before all world objects, or the entire terrain looks blank.
        // The original 16x33 Array is physically transposed: row 18
        // begins the second terrain half toward world X, not world Y.
        const float source_second_half_x=world_.grid().height()==33
            ? minidayz::world_source::mapgen_hotspot(0,18).x : 1.0e30f;
        if (!renderer.draw_original_ground_floor(
                camera_x, camera_y, view_w, view_h,
                world_.generation_level(),source_second_half_x)) {
            renderer.draw_solid_rect(camera_x, camera_y, view_w, view_h,
                                     C2D_Color32(80, 72, 62, 255));
            renderer.draw_text("ORIGINAL FLOOR TEXTURE FAILED TO LOAD",
                               camera_x + 12.0f, camera_y + 12.0f, 0.30f);
        }

        // Reserve the actual selected actor pages before scenery can fill
        // the 32-page GPU cache. The second call submits the same original
        // source frames at their actual layer, not before the background.
        const auto draw_npcs=[&](){
        if (npc_ready_) {
            // Root Map layer29 ("Zeds") is visible; layer49
            // ("zed_resp") is explicitly HIDDEN. Source Spawn_NPC
            // creates each real body on Zeds but its eyes/controller
            // Sprite256 on zed_resp. Only the body artwork belongs in
            // the 3DS world draw; the controller still exists in NPC
            // logic and must NOT be painted in front of the zombie.
            const float npc_body_depth=
                minidayz::world_source::source_world_layer_depth(29,0,0);
            for (std::size_t i = 0; i < npcs_.count(); ++i) {
                const auto* npc = npcs_.actor(i);
                if (!npc || !npc->alive ||
                    npc->x < camera_x - 48.0f || npc->x > camera_x + view_w + 48.0f ||
                    npc->y < camera_y - 48.0f || npc->y > camera_y + view_h + 48.0f)
                    continue;
                // Original NPC skin or no rendered NPC: a colored native
                // rectangle is not an authored MiniDAYZ character frame.
                (void)renderer.draw_source_sprite(npc->source_object,
                    npc->animation_index, npc->animation_frame,
                    npc->x, npc->y, 1.0f, 1.0f,npc_body_depth);
            }
        }

        };
        const auto draw_player=[&](bool reserve_only){
        // Match original Construct Map layer 32 (player) and source
        // clothing/weapon layer indices 33..39, not hardcoded 3DS Z.
        const float player_depth=minidayz::stage1::source_player_layer_depth(
            minidayz::stage1::kSourcePlayerLayer);
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
        // The PlayerSystem selects and validates the original Construct
        // animation source at spawn/restore time. Render THAT same source
        // object; using an independent menu index here can draw a different
        // skin than the one whose animation frames PlayerSystem advances.
        const auto source_skin_object=player_.source_skin();
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
        const bool outlined=source_player_outline_ &&
            minidayz::stage1::source_family_contains(1064,source_skin_object);
        bool sprite_drawn=false;
        if(!p.alive) {
            // Original Construct death events spawn Sprite813 and select
            // "id_" + source survivor skin type. Use only animations
            // verified against the ROOT Game_events 193 skin assignment.
            // The source 21st skin has no authored "id_21" corpse art.
            const auto* original_dead_animation=
                minidayz::stage1::source_dead_skin_animation(
                    source_skin_object);
            if(original_dead_animation) {
                const auto corpse_frame=
                    minidayz::stage1::source_equipment_animation_frame(
                        813,original_dead_animation,
                        source_dead_animation_elapsed_);
                sprite_drawn=renderer.draw_source_animation(
                    813,original_dead_animation,corpse_frame,
                    p.x,p.y,1.0f,1.0f,
                    minidayz::world_source::source_world_layer_depth(18,0,1));
            }
        }
        if(p.alive && weapon_pose &&
            std::strcmp(weapon_pose,"idle")!=0 &&
            std::strcmp(weapon_pose,"run")!=0) {
            char armed_animation[32]{};
            std::snprintf(armed_animation,sizeof(armed_animation),"%s_%s",
                          weapon_pose,skin_direction);
            // An original weapon stance is a DIFFERENT Construct Sprite
            // animation from run_*/idle_*. Resolve its own authored speed,
            // frame count and looping, not the survivor's run frame number.
            const auto weapon_stance_frame =
                minidayz::stage1::source_equipment_animation_frame(
                    source_skin_object,armed_animation,p.animation_elapsed);
            sprite_drawn=renderer.draw_source_animation(source_skin_object,
                armed_animation,weapon_stance_frame,
                p.x,p.y,1.0f,1.0f,player_depth,0.0f,outlined);
        }
        if(p.alive && !sprite_drawn)
            sprite_drawn=renderer.draw_source_animation(source_skin_object,
                skin_animation,player_.source_animation_frame(),
                p.x,p.y,1.0f,1.0f,player_depth,0.0f,outlined);
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
                p.x,p.y,1.0f,1.0f,player_depth,0.0f,
                source_player_outline_ && minidayz::stage1::source_family_contains(
                    1064,minidayz::kSourcePlayerObject));
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
                // Overlay animation metadata belongs to the original
                // EQUIPMENT sprite, not to the selected survivor's skin.
                const auto original_frame =
                    minidayz::stage1::source_equipment_animation_frame(
                        visual->object,name,p.animation_elapsed);
                (void)renderer.draw_source_animation(visual->object,name,
                    original_frame,p.x,p.y,1.0f,1.0f,depth);
            };
            // Source-authored equipment back-to-front. The originals use
            // separate layers on top of the chosen character base skin.
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::pants),false,minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerPantsLayer));
            // The original Start events pair every PlayerBase193 skin
            // sprite (instance var0) with a SEPARATE matching hands Sprite
            // (instance var1). The old 3DS renderer entirely omitted that
            // animation layer, leaving the survivor's hands/weapon poses
            // wrong even when the body texture was the correct PNG.
            // The pairs and original 21 skin types come from root events,
            // never guessed from matching asset filenames or skin color.
            const auto source_hands =
                minidayz::stage1::source_player_hands_object(source_skin_object);
            if(source_hands!=0) {
                const auto hands_depth=minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerHandsLayer);
                const auto draw_original_hands=[&](const char* animation) noexcept {
                    const auto hand_frame =
                        minidayz::stage1::source_equipment_animation_frame(
                            source_hands,animation,p.animation_elapsed);
                    return renderer.draw_source_animation(source_hands,animation,
                        hand_frame,p.x,p.y,1.0f,1.0f,hands_depth);
                };
                bool hand_drawn=false;
                if(weapon_pose &&
                   std::strcmp(weapon_pose,"idle")!=0 &&
                   std::strcmp(weapon_pose,"run")!=0) {
                    char armed_hands[32]{};
                    std::snprintf(armed_hands,sizeof(armed_hands),"%s_%s",
                                  weapon_pose,skin_direction);
                    hand_drawn=draw_original_hands(armed_hands);
                }
                if(!hand_drawn)
                    (void)draw_original_hands(skin_animation);
            }
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::outerwear),false,minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerClothingLayer));
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::vest),false,minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerVestLayer));
            draw_equipped_layer(equipment_.equipped(
                minidayz::items::EquipmentSlot::head),false,minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerHelmetLayer));
            draw_equipped_layer(inventory_.equipped_backpack(),false,minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerBackpackLayer));
            // Original satchels are separate wearable storage equipment,
            // not part of the main backpack slot. Show both when equipped.
            draw_equipped_layer(inventory_.equipped_satchel(),false,minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerBackpackLayer));
            // Draw only the selected held firearm OR melee overlay, not
            // both weapons through the same hand at the same time.
            draw_equipped_layer(held_item,true,minidayz::stage1::source_player_layer_depth(
                    minidayz::stage1::kSourcePlayerWeaponLayer));
        }
        if(p.alive && !sprite_drawn && !reserve_only) {
            // Never conceal a missing source atlas behind an invented
            // colored player block. The source default skin is the genuine
            // last-resort artwork; report a load problem explicitly.
            renderer.draw_text("ORIGINAL PLAYER SPRITE MISSING",
                p.x-60.0f,p.y+16.0f,0.23f);
        }

        };
        renderer.set_source_texture_reservation(true);
        draw_player(true);
        draw_npcs();
        renderer.set_source_texture_reservation(false);
        // Keep source rain/night atlas pages resident in GPU memory before
        // dense scenery needs additional atlas slots.
        const float night_opacity=minidayz::items::source_outdoor_night_opacity(
            day_night_.hour(),day_night_.minute(),
            perks_.level(minidayz::items::SourcePerk::eye)==2?2.0f:0.0f);
        // This scene currently generates outdoor Map only. Bunker/FinalStand
        // entry must pass its actual scene state when those transitions are ported.
        const bool night_layer_visible=minidayz::items::source_night_layer_visible(
            day_night_.hour(),false);
        // These calls do not submit pixels or alter the source event state;
        // actual alpha overlays still draw AFTER scenery at Map layers44/46.
        if (rain_timer_.active()) {
            (void)renderer.pin_source_sprite_texture(
                minidayz::stage1::kOriginalRainFadeArt.object,
                minidayz::stage1::kOriginalRainFadeArt.animation,
                minidayz::stage1::kOriginalRainFadeArt.frame);
        }
        if(twilight_fade_.opacity()>0.0f)
            (void)renderer.pin_source_sprite_texture(
                minidayz::items::SourceTwilightFade::kObject,0,twilight_fade_.frame());
        if (night_layer_visible) {
            (void)renderer.pin_source_sprite_texture(
                minidayz::stage1::kOriginalNightOverlayArt.object,
                minidayz::stage1::kOriginalNightOverlayArt.animation,
                minidayz::stage1::kOriginalNightOverlayArt.frame);
        }

        if (world_ready_) {
            // WorldGrid codes 12/13/14 are the original procedural road
            // network. They must be drawn before buildings and trees, or
            // even a correctly loaded ground sheet shows an empty wilderness.
            if (!renderer.draw_source_road_grid(world_,
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
            if (!renderer.draw_source_ponds(world_,camera_x,camera_y,view_w,view_h)) {
                renderer.draw_text("PROCEDURAL WATER TILES FAILED",
                                   camera_x+12.0f,camera_y+68.0f,0.30f);
            }
            // Original Game_events scatter source Tilemap object 230 per
            // generated location cell. These are real 30px sprites from
            // assets/gfx/ground_enviroment_tilemap.png, not placeholder art.
            if (!renderer.draw_source_procedural_environment(world_,
                    camera_x,camera_y,view_w,view_h)) {
                renderer.draw_text("PROCEDURAL TERRAIN TILES FAILED",
                                   camera_x+12.0f,camera_y+54.0f,0.30f);
            }
            const auto& special_terrain=world_.source_special_terrain();
            if(!special_terrain.empty() && !renderer.draw_source_special_terrain(
                    special_terrain,camera_x,camera_y,view_w,view_h,
                    minidayz::world_source::source_world_layer_depth(
                        special_terrain.layer,0,0))){
                renderer.draw_text("ORIGINAL SPECIAL TERRAIN FAILED",
                                   camera_x+12.0f,camera_y+82.0f,0.30f);
            }
        }

        if(!world_ready_)visible_world_placements_.clear();
            // Render the actual source Map layout in world coordinates.
            // Cull placements outside the camera rectangle to keep 3DS draw
            // cost bounded instead of drawing the entire 36k x 20k layout.
            const auto* source_map = minidayz::stage1::find_layout("Map");
            const auto source_map_index = source_map
                ? static_cast<std::uint16_t>(
                      source_map - minidayz::stage1::kLayoutTable)
                : 0;
            if(world_ready_)visibility_index_.query(camera_x,camera_y,view_w,view_h,
                                    visible_world_placements_);
            visible_world_placements_.erase(std::remove_if(
                visible_world_placements_.begin(),visible_world_placements_.end(),
                [&](std::uint32_t i){const auto* p=world_.placement(i);
                    return !p || !world_.source_placement_active(*p);}),
                visible_world_placements_.end());
            std::size_t world_source_requested=0;
            std::size_t world_source_drawn=0;
            std::size_t world_source_bad_frames=0;
            // Construct paints by Map layer and then instance creation order.
            // The source roof events can move an instance between layers:
            // sort the currently visible instances AFTER updating their layer.
            minidayz::world_source::source_sort_visible_layer_order(
                visible_world_placements_,[&](std::uint32_t id) noexcept {
                    const auto* placed=world_.placement(id);
                    return placed?placed->layer:std::uint16_t{0xffff};
                });
            std::size_t layer_begin=0,layer_end=0;
            std::size_t visible_slot=0;
        const auto draw_world_through=[&](std::uint16_t last_layer){
            for(;visible_slot<visible_world_placements_.size();++visible_slot) {
                const auto i=visible_world_placements_[visible_slot];
                const auto* placement=world_.placement(i);
                if(!placement)continue;
                if(placement->layer>last_layer)break;
                if(visible_slot>=layer_end){
                    layer_begin=visible_slot;
                    layer_end=visible_slot+1;
                    while(layer_end<visible_world_placements_.size()){
                        const auto* next=world_.placement(
                            visible_world_placements_[layer_end]);
                        if(!next || next->layer!=placement->layer)break;
                        ++layer_end;
                    }
                }
                // Original create_map_element can place gameplay artwork
                // on high Map layers. Require its exact source event UID
                // and art/layer pair rather than hiding those source frames
                // or admitting unrelated conditional GUI overlays.
                // Original source Tilemaps, Arrays and invisible trigger
                // objects must not be drawn as bogus marker rectangles.
                const auto* source_object =
                    minidayz::stage1::find_object(placement->object);
                if (!source_object || source_object->type != 18) continue;
                // Root Sprite404 is ach_city_counter-sheet0.png: a tiny
                // magenta gameplay REGION MARKER, not building artwork.
                // Construct creates it as a 1000x1000 area on Buildings41;
                // drawing that counter as a sprite produced the enormous
                // solid purple blocks visible in Manic Emu screenshots.
                // Keep the placement and UID for source achievements/
                // proximity logic, but never submit its pixels to the GPU.
                if (minidayz::world_source::source_world_logic_marker_not_art(
                        placement->object) ||
                    minidayz::world_source::source_world_preview_needs_live_event(
                        placement->object)) continue;
                const auto* source_layer = source_map
                    ? minidayz::stage1::find_layer(
                          source_map_index, placement->layer)
                    : nullptr;
                // Root source object241 lives on Sunset_Dawn, not the
                // generic GUI/browser overlays. Resolve the Map layer NAME
                // so rebuilt original-source layer indices stay correct.
                if(!source_layer ||
                   (placement->layer>=44 &&
                    !minidayz::world_source::source_world_high_layer_gameplay_art(
                        placement->object,source_layer->name,
                        placement->source_uid)))continue;
                if (source_layer && !source_layer->visible) continue;
                // Root night layer visibility events hide its children during daylight.
                if(placement->layer==46 && !night_layer_visible)continue;
                // Sprite663 is a destination-out mask, not visible foreground art.
                // Its source pixels are consumed by draw_source_night_mask below.
                if(placement->layer==46 && placement->object==663)continue;
                if (source_layer &&
                    (std::strstr(source_layer->name, "respawn") ||
                     std::strstr(source_layer->name, "_resp") ||
                     std::strstr(source_layer->name, "points"))) continue;
                // Original UI player sprite belongs to runtime-controlled
                // survivor rendering below, not a permanent source Map image.
                if (placement->layer == 32 || placement->opacity <= 0.0f)
                    continue;
                // The original variant112 shadow has an intentional zero
                // height (SetHeight(s2.Y-s1.Y)). Do not stretch a zero-area
                // source object back to its full atlas frame in rendering.
                if (placement->object==347 && placement->height==0.0f)continue;
                // Use the original Construct instance position and frame
                // coordinates. Width and height may be scaled differently:
                // a single scale_x for both axes distorted buildings,
                // foliage, vehicles and authored world decorations.
                const float px = placement->x;
                const float py = placement->y;
                const float authored_sx = placement->scale_x != 0.0f
                    ? placement->scale_x : 1.0f;
                const auto key=std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{
                    placement->object,placement->source_animation,placement->source_frame};
                const auto* begin=minidayz::stage1::kSpriteFrames;
                const auto* end=begin+minidayz::stage1::kSpriteFrameCount;
                const auto* sprite=std::lower_bound(begin,end,key,
                    [](const minidayz::stage1::SpriteFrameDef& item,
                       const auto& wanted){
                        return std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{
                            item.object,item.animation,item.frame}<wanted;
                    });
                const bool source_frame_valid =
                    sprite!=end && sprite->object==placement->object &&
                    sprite->animation==placement->source_animation &&
                    sprite->frame==placement->source_frame;
                const float sx=source_frame_valid && sprite->width>0.0f &&
                        placement->width>0.0f
                    ? std::copysign(placement->width/sprite->width,authored_sx)
                    : authored_sx;
                const float sy=source_frame_valid && sprite->height>0.0f &&
                        placement->height>0.0f
                    ? placement->height/sprite->height : std::fabs(authored_sx);
                if(!source_frame_valid)++world_source_bad_frames;
                if (source_frame_valid) {
                    if (!minidayz::terrain::source_sprite_in_view(
                        px,py,sprite->width,sprite->height,
                        sprite->origin_x,sprite->origin_y,sx,sy,
                        placement->angle*0.01745329251994329577f,
                        camera_x,camera_y,view_w,view_h)) continue;
                } // Keep unknown frame dimensions visible for diagnostics.
                // Manic Emu confirms larger Z is in front. The original
                // Map 31 Buildings_bg stays behind the player, while 41
                // Buildings and 43 Trees stay above. Distinct depths also
                // preserve the original insertion order WITHIN each layer
                // instead of relying on undefined equal-depth GPU ordering.
                const bool tree_over_player=p.alive &&
                    minidayz::terrain::source_tree_occludes_player(
                        *placement,p.x,p.y);
                const float layer_depth=minidayz::terrain::source_tree_render_depth(
                    minidayz::world_source::source_world_layer_depth(
                        placement->layer,visible_slot-layer_begin,layer_end-layer_begin),
                    minidayz::stage1::source_player_layer_depth(
                        minidayz::stage1::kSourcePlayerLayer),
                    tree_over_player);
                ++world_source_requested;
                if (renderer.draw_source_sprite(placement->object,
                        placement->source_animation, placement->source_frame,
                        px, py, sx, sy, layer_depth,
                        placement->angle * 0.01745329251994329577f,
                        tree_over_player ? std::min(placement->opacity,0.36f)
                                         : placement->opacity,
                        i<source_foundation_tint_.size() && source_foundation_tint_[i]?
                            0.5f:1.0f)) {
                    ++world_source_drawn;
                }
            }

        };

        // Original Map order: base17, drops18, NPC29, player32..39,
        // foreground Buildings41, Trees43, then Rain44 and night46.
        // Submit back-to-front as well as using depth: alpha blending
        // otherwise loses background pixels beneath translucent sprites.
        draw_world_through(17);
        for (std::size_t i = 0; i < tents_.tent_count(); ++i) {
            const auto* tent = tents_.tent(i);
            if (!tent || tent->x < camera_x - 49.0f ||
                tent->x > camera_x + view_w + 49.0f ||
                tent->y < camera_y - 65.0f ||
                tent->y > camera_y + view_h + 65.0f) continue;
            (void)renderer.draw_source_sprite(398,0,0,tent->x,tent->y,
                1.0f,1.0f,minidayz::world_source::source_dynamic_layer_depth(17,i,tents_.tent_count()));
        }
        draw_world_through(18);
        for (std::size_t i = 0; i < stashes_.count(); ++i) {
            const auto* stash = stashes_.stash(i);
            if (!stash || stash->x < camera_x - 24.0f ||
                stash->x > camera_x + view_w + 24.0f ||
                stash->y < camera_y - 24.0f ||
                stash->y > camera_y + view_h + 24.0f) continue;
            (void)renderer.draw_item_icon(
                minidayz::items::kOriginalSuppliesStashItemId,
                stash->x, stash->y, 28.0f, 28.0f, minidayz::world_source::source_dynamic_layer_depth(18,i,stashes_.count()+ground_loot_.count()));
        }

        for (std::size_t i = 0; i < ground_loot_.count(); ++i) {
            const auto* loot = ground_loot_.entry(i);
            if (!loot || loot->x < camera_x - 24.0f ||
                loot->x > camera_x + view_w + 24.0f ||
                loot->y < camera_y - 24.0f ||
                loot->y > camera_y + view_h + 24.0f) continue;
            (void)renderer.draw_item_icon(loot->item.item_id,
                loot->x, loot->y, 18.0f, 18.0f, minidayz::world_source::source_dynamic_layer_depth(18,stashes_.count()+i,stashes_.count()+ground_loot_.count()));
        }

        if(!p.alive)draw_player(false);
        draw_world_through(29);
        draw_npcs();
        draw_world_through(31);
        if(p.alive)draw_player(false);
        draw_world_through(41);
        if(world_ready_){
            // Root event 622329722190205 also emits the original
            // object317 from test_bullet_hit-sheet0.png on Buildings41,
            // before Wait(1) and item50/hide restitution.
            if(source_bed_dismantle_.runtime_uid) {
                const auto* dying=world_.placement(source_bed_dismantle_.index);
                if(dying && dying->runtime_uid==source_bed_dismantle_.runtime_uid)
                    (void)renderer.draw_source_sprite(317,0,0,
                        dying->x,dying->y,1.0f,1.0f,
                        minidayz::world_source::source_world_layer_depth(41,0,1));
            }

            // The original Building_mode(12) spawns object573 Ghost frame12
            // on Buildings layer41; use its exact original atlas rectangle.
            if(source_bed_build_.active) {
                const auto& ghost=source_bed_build_;
                const bool valid=source_bed_build_site_valid(ghost.x,ghost.y);
                (void)renderer.draw_source_sprite(
                    minidayz::world_source::kSourceBedGhostObject,0,
                    minidayz::world_source::kSourceBedGhostFrame,
                    ghost.x,ghost.y,1.0f,1.0f,
                    minidayz::world_source::source_world_layer_depth(41,0,1),
                    0.0f,valid?0.90f:0.45f);
                renderer.draw_text(ghost.confirmed?
                    "BED: BUILDING (3s)":
                    valid?"A PLACE / B CANCEL":"NEAR HOUSE WALL: A CHECK",
                    ghost.x-45.0f,ghost.y-34.0f,0.18f);
            }

            // Bridge source features are separate from native placement records.
            // Bunker feature975 is z_spawner metadata, not entrance art.
            for (std::size_t i = 0; i < world_.feature_count(); ++i) {
                const auto* feature = world_.feature(i);
                if (!feature) continue;
                const bool bridge = feature->kind ==
                    minidayz::WorldFeatureKind::Bridge &&
                    (feature->variant & 1u) != 0u;
                if (!bridge || minidayz::world_source::source_world_logic_marker_not_art(feature->source_object)) continue;
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
                    1.0f, 1.0f, minidayz::world_source::source_world_layer_depth(feature->source_layer,0,1));
            }
        }
        for (std::size_t i = 0; i < tents_.tent_count(); ++i) {
            const auto* tent = tents_.tent(i);
            if (!tent || tent->x < camera_x - 49.0f ||
                tent->x > camera_x + view_w + 49.0f ||
                tent->y < camera_y - 65.0f ||
                tent->y > camera_y + view_h + 65.0f) continue;
            // Root deployment events 297398672474814 / 1391504325547996:
            // Sprite398 is the base; Sprite397 is the separate roof.
            // The renderer selects each original atlas rectangle (49x65),
            // never the entire civilian_tent_deploy_up sprite sheet.
            minidayz::WorldPlacement roof{};
            roof.object=397;
            roof.x=tent->x;roof.y=tent->y;
            roof.width=49.0f;roof.height=65.0f;
            roof.scale_x=1.0f;
            const minidayz::WorldPoint actor{player_.state().x,player_.state().y};
            // Root events 3450973739819458 / 8566394378489923.
            // Exact source frame vertices relative to its (25,33) hotspot:
            // (-25,-33), (12,-33), (12,32), (-25,32).
            const bool overlap=actor.x+15.0f>=roof.x-25.0f &&
                               actor.x-15.0f<=roof.x+12.0f &&
                               actor.y+15.0f>=roof.y-33.0f &&
                               actor.y-15.0f<=roof.y+32.0f;
            (void)renderer.draw_source_sprite(397,0,0,tent->x,tent->y,
                                               1.0f,1.0f,minidayz::world_source::source_dynamic_layer_depth(41,i,tents_.tent_count()),0.0f,
                                               overlap?0.25f:1.0f);
        }

        draw_world_through(43);
        if (rain_timer_.active()) {
            (void)renderer.draw_source_sprite_fitted(
                minidayz::stage1::kOriginalRainFadeArt.object,
                minidayz::stage1::kOriginalRainFadeArt.animation,
                minidayz::stage1::kOriginalRainFadeArt.frame,
                camera_x, camera_y, view_w, view_h,
                minidayz::world_source::source_world_layer_depth(44,0,0));
        }
        draw_world_through(45);
        if (night_layer_visible && !renderer.draw_source_night_mask(world_,
                visible_world_placements_,camera_x,camera_y,night_opacity,
                minidayz::world_source::source_world_layer_depth(46,0,0))) {
            (void)renderer.draw_source_sprite_fitted(
                minidayz::stage1::kOriginalNightOverlayArt.object,
                minidayz::stage1::kOriginalNightOverlayArt.animation,
                minidayz::stage1::kOriginalNightOverlayArt.frame,
                camera_x, camera_y, view_w, view_h,
                minidayz::world_source::source_world_layer_depth(46,0,0),night_opacity);
        }
        draw_world_through(0xffff);
        // Original GUI Color_effect Sprite971 is pinned to the view, above Map/night.
        // The original 2.5-device-size rectangle covers the entire viewport.
        if(twilight_fade_.opacity()>0.0f)
            (void)renderer.draw_source_sprite_fitted(
                minidayz::items::SourceTwilightFade::kObject,0,twilight_fade_.frame(),
                camera_x,camera_y,view_w,view_h,0.82f,twilight_fade_.opacity());
        renderer.set_world_sprite_stats(
            visible_world_placements_.size(),world_source_requested,
            world_source_drawn,world_source_bad_frames);
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

    void rebuild_source_house_roofs() {
        source_roof_indices_.clear();
        source_house_interior_indices_.clear();
        source_house_wall_indices_.clear();
        source_house_foundation_indices_.clear();
        for(std::size_t i=0;i<world_.placement_count();++i){
            const auto* p=world_.placement(i);
            if(!p)continue;
            if(minidayz::stage1::source_family_contains(1049,p->object))
                source_roof_indices_.push_back(static_cast<std::uint32_t>(i));
            if(minidayz::stage1::source_family_contains(1041,p->object))
                source_house_interior_indices_.push_back(static_cast<std::uint32_t>(i));
            if(minidayz::stage1::source_family_contains(1048,p->object))
                source_house_foundation_indices_.push_back(static_cast<std::uint32_t>(i));
            if(p->object==346)
                source_house_wall_indices_.push_back(static_cast<std::uint32_t>(i));
        }
        source_roof_background_layer_=source_roof_buildings_layer_=
            source_roof_base_layer_=source_roof_base_top_layer_=0xffff;
        if(const auto* map=minidayz::stage1::find_layout("Map")){
            const auto layout=static_cast<std::uint16_t>(
                map-minidayz::stage1::kLayoutTable);
            for(const auto& layer:minidayz::stage1::kLayerTable){
                if(layer.layout!=layout)continue;
                if(std::strcmp(layer.name,"Buildings_bg")==0)
                    source_roof_background_layer_=layer.index;
                else if(std::strcmp(layer.name,"Buildings")==0)
                    source_roof_buildings_layer_=layer.index;
                else if(std::strcmp(layer.name,"Buildings_base")==0)
                    source_roof_base_layer_=layer.index;
                else if(std::strcmp(layer.name,"Buildings_base_top")==0)
                    source_roof_base_top_layer_=layer.index;
            }
        }
        source_roof_elapsed_=source_interior_elapsed_=0.0f;
        source_overlapping_wall_indices_.clear();
        source_foundation_tint_.assign(world_.placement_count(),0u);
        source_player_outline_=false;
    }

    void update_source_house_roofs(float dt) noexcept {
        if(!world_ready_ || !player_.state().alive)return;
        using namespace minidayz::world_source;
        const float step=std::max(0.0f,dt);
        source_roof_elapsed_+=step;
        source_interior_elapsed_+=step;
        const minidayz::WorldPoint player_pos{player_.state().x,player_.state().y};
        // Source family1041 has its own 0.5 second layer event, not
        // the foundation-selection condition of the roof-opacity branch.
        if(source_interior_elapsed_>=0.5f){
            source_interior_elapsed_=std::fmod(source_interior_elapsed_,0.5f);
            for(const auto i:source_house_interior_indices_){
                const auto* p=world_.placement(i);
                if(!p || !world_.source_placement_active(*p) || !source_player_overlaps_world_sprite(
                        *p,player_pos,p->origin_x,p->origin_y))continue;
                const auto layer=player_pos.y>p->y?
                    source_roof_background_layer_:source_roof_buildings_layer_;
                if(layer!=0xffff && p->layer!=layer)
                    (void)world_.set_placement_layer(i,layer);
            }
        }
        if(source_roof_elapsed_<0.1f)return;
        source_roof_elapsed_=std::fmod(source_roof_elapsed_,0.1f);
        // 193 is the authored 2x2 PlayerBase with hotspot (1,1);
        // 181 is the separate 30x30 collision Sprite. The original
        // wall-zone condition uses 193, NOT the 30px collision rectangle.

        // Retain the picked wall instances, not a global boolean: an
        // adjacent building must not inherit another house's wall zone.
        source_overlapping_wall_indices_.clear();
        for(const auto i:source_house_wall_indices_){
            const auto* wall=world_.placement(i);
            if(wall && world_.source_placement_active(*wall) && source_player_base_overlaps_wall(*wall,player_pos)){
                source_overlapping_wall_indices_.push_back(i);
            }
        }
        bool sheltered=false;
        bool any_roof_overlap=false;
        for(const auto index:source_roof_indices_){
            const auto* roof=world_.placement(index);
            if(!roof || !world_.source_placement_active(*roof))continue;
            const auto* frame=source_roof_member_point(
                roof->object,roof->source_animation,roof->source_frame);
            if(!frame)continue;
            const bool overlap=source_player_overlaps_world_sprite(
                *roof,player_pos,frame->origin_x,frame->origin_y);
            any_roof_overlap=any_roof_overlap || overlap;
            // Root event 6583602733022543 (System Pick nearest 1048)
            // selects the foundation at the roof's X/Y. Do not substitute
            // the procedural parent UID for this source picking condition.
            if(!overlap && roof->opacity==1.0f &&
               roof->layer==source_roof_background_layer_)continue;
            std::size_t base_index=world_.placement_count();
            float best=std::numeric_limits<float>::max();
            for(const auto i:source_house_foundation_indices_){
                const auto* p=world_.placement(i);
                if(!p || !world_.source_placement_active(*p))continue;
                const float dx=p->x-roof->x,dy=p->y-roof->y;
                const float d=dx*dx+dy*dy;
                if(d<best){best=d;base_index=i;}
            }
            const auto* base=world_.placement(base_index);
            const bool has_base=base!=nullptr;
            // Original 8794469330826744 selects foundations by actual
            // overlap with the picked 346 wall-zone, not generated UIDs.
            // Root UID 8794469330826744 picks a 1048 foundation
            // overlapping the SAME wall346 that the player overlaps.
            // Do not let a wall in another house fade this roof: the old
            // code checked ANY foundation here, so every touched wall
            // could force every neighboring overlapped roof transparent.
            const bool player_in_wall=!source_overlapping_wall_indices_.empty();
            bool touching_wall=false;
            if(overlap && base){
                for(const auto w:source_overlapping_wall_indices_){
                    const auto* wall=world_.placement(w);
                    if(wall && source_picked_house_foundation_overlaps_wall(
                            *base,*wall)){
                        touching_wall=true;
                        break;
                    }
                }
            }
            // Original root Rain event UID 1706727368417029 is a
            // sibling of the 1048-foundation pick: PlayerBase193 touching
            // ANY wall346 while under 1049 is enough for sheltered rain.
            sheltered=sheltered || (overlap && player_in_wall);
            auto decision=source_house_roof_decision(
                *roof,*frame,player_pos,has_base,touching_wall);
            // The wall-zone branch does not evaluate the outside thresholds.
            if(overlap && player_in_wall){
                decision.opacity=touching_wall?0.0f:roof->opacity;
                decision.move_background=touching_wall ||
                    roof->layer==source_roof_background_layer_;
            }
            if(overlap && !player_in_wall && base){
                const auto* anim=source_roof_foundation_animations(base->object);
                if(anim && base->source_animation!=anim->out_animation)
                    (void)world_.set_placement_sprite_state(
                        base_index,anim->out_animation,0,base->opacity);
            }
            if(std::fabs(roof->opacity-decision.opacity)>0.001f)
                (void)world_.set_placement_sprite_state(
                    index,roof->source_animation,roof->source_frame,decision.opacity);
            const auto roof_layer=decision.move_background?
                source_roof_background_layer_:source_roof_buildings_layer_;
            if(roof_layer!=0xffff && roof->layer!=roof_layer)
                (void)world_.set_placement_layer(index,roof_layer);
            // Root events 4153213040695326 / 7058744064425045
            // pick the nearest roof to the selected foundation again.
            // Do not change another house's foundation layer if that
            // reverse pick selects a different roof.
            bool reverse_pick_matches=false;
            if(base){
                float nearest=std::numeric_limits<float>::max();
                std::size_t picked=world_.placement_count();
                for(const auto i:source_roof_indices_){
                    const auto* candidate=world_.placement(i);
                    if(!candidate || !world_.source_placement_active(*candidate))continue;
                    const float dx=candidate->x-base->x,dy=candidate->y-base->y;
                    const float d=dx*dx+dy*dy;
                    if(d<nearest){nearest=d;picked=i;}
                }
                reverse_pick_matches=picked==index;
            }
            if(base && reverse_pick_matches && !player_in_wall &&
               (overlap || base->layer==source_roof_base_top_layer_)){
                const auto base_layer=decision.foundation_top?
                    source_roof_base_top_layer_:source_roof_base_layer_;
                if(base_layer!=0xffff && base->layer!=base_layer)
                    (void)world_.set_placement_layer(base_index,base_layer);
                if(overlap){
                    // Original effects 7294047492931295 / 8381343549210222:
                    // Map t1048 members author Tint RGB=(0.5,0.5,0.5).
                    // 181 instance variable 42 enables family 1064 Outline.
                    if(base_index<source_foundation_tint_.size())
                        source_foundation_tint_[base_index]=decision.foundation_top?1u:0u;
                    source_player_outline_=decision.foundation_top;
                }else if(base_index<source_foundation_tint_.size() &&
                         source_house_sprites_overlap(*base,*roof)){
                    source_foundation_tint_[base_index]=0u;
                }
            }
            // Original events 6437471853562815 / 7764770537466591
            // pick door 349 by mask overlap with the roof or wall-zone.
            if(overlap && (!player_in_wall || touching_wall)){
                for(const auto i:source_door_indices_){
                    const auto* door=world_.placement(i);
                    if(!door || !world_.source_placement_active(*door))continue;
                    bool picked=!player_in_wall &&
                        source_house_sprites_overlap(*door,*roof);
                    if(touching_wall && base){
                        for(const auto w:source_overlapping_wall_indices_){
                            const auto* wall=world_.placement(w);
                            if(wall && source_picked_house_foundation_overlaps_wall(
                                    *base,*wall) &&
                               source_house_sprites_overlap(*door,*wall)){
                                picked=true;break;
                            }
                        }
                    }
                    if(!picked)continue;
                    const float opacity=player_in_wall?0.25f:1.0f;
                    if(std::fabs(door->opacity-opacity)>0.001f)
                        (void)world_.set_placement_sprite_state(
                            i,door->source_animation,door->source_frame,opacity);
                }
            }
        }
        // Event 9848716569903957 clears the Outline flag when no roof
        // is overlapped, and child 7529401044727926 disables Tint.
        if(!any_roof_overlap){
            source_player_outline_=false;
            std::fill(source_foundation_tint_.begin(),source_foundation_tint_.end(),0u);
        }
        if(rain_timer_.active())stage2_audio_.set_rain_sheltered(sheltered);
    }


    // The source does not encode arbitrary furniture searches: actual
    // buildable beds and water pumps are separate Sprite951 / Sprite442.
    // The house interior furniture is often painted into a single original
    // base PNG and cannot legitimately be treated as separate items.
    void rebuild_source_furniture() noexcept {
        source_furniture_indices_.clear();
        source_bed_build_={};
        source_bed_dismantle_={};
        source_sleep_={};
        source_pump_pending_={};
        source_bed_selected_uid_=0;
        source_furniture_feedback_seconds_=0.0f;
        source_furniture_feedback_="";
        for(std::size_t i=0;i<world_.placement_count();++i){
            const auto* p=world_.placement(i);
            if(p && minidayz::world_source::source_furniture_active(*p))
                source_furniture_indices_.push_back(
                    static_cast<std::uint32_t>(i));
        }
    }

    // Original Player181/PlayerBase193 interact with Sprite241 trunk at
    // source distance <= 100 (UID711142196261088). Loot is conditional;
    // searching always
    // destroys the original trunk and awards two source XP.
    bool request_source_car_trunk() noexcept {
        const float x=player_.state().x,y=player_.state().y;
        float distance=minidayz::items::kOriginalCarTrunkInteractionRange;
        std::size_t nearest=world_.placement_count();
        for(std::size_t i=0;i<world_.placement_count();++i){
            const auto* p=world_.placement(i);
            if(!p || !world_.source_placement_active(*p) || p->object!=241 || !p->runtime_uid ||
               p->source_variable_0!=0 || p->opacity<=0.0f)continue;
            const float d=std::hypot(p->x-x,p->y-y);
            if(d<=distance){distance=d;nearest=i;}
        }
        if(nearest==world_.placement_count())return false;
        const auto outcome=minidayz::items::source_search_original_car_trunk(
            world_,nearest,ground_loot_,perks_.source_trunk_bonus(),
            perks_.level(minidayz::items::SourcePerk::trunkster),
            world_.source_random());
        using Result=minidayz::items::OriginalCarSearchResult;
        if(outcome==Result::invalid)return false;
        if(outcome==Result::capacity_failure){
            source_furniture_feedback_="CAR TRUNK: GROUND LOOT FULL";
            source_furniture_feedback_seconds_=3.0f;
            return true;
        }
        // Root Game_events UID 7711324247664447 plays "open" (-5dB)
        // ONCE when Sprite241 is searched. Do not duplicate the source
        // cue or substitute a door/car sample.
        stage2_audio_.play_original_world_sound(
            minidayz::OriginalWorldSound::carTrunkOpen);
        (void)perks_.award_experience(2);
        source_furniture_feedback_=outcome==Result::found
            ?"CAR TRUNK: LOOT FOUND":"CAR TRUNK: EMPTY";
        source_furniture_feedback_seconds_=3.0f;
        return true;
    }

    // Called when A is pressed with no inventory selection, after the
    // original manual door event has had priority.
    void request_source_furniture() noexcept {
        using namespace minidayz::world_source;
        const minidayz::WorldPoint actor{
            player_.state().x,player_.state().y};
        std::size_t chosen=world_.placement_count();
        float best=std::numeric_limits<float>::max();
        for(auto idx:source_furniture_indices_){
            const auto* p=world_.placement(idx);
            if(!p || !world_.source_placement_active(*p) || !source_furniture_active(*p))continue;
            if(!source_player_overlaps_world_sprite(*p,actor,
                    p->origin_x,p->origin_y))continue;
            const float d=std::hypot(p->x-actor.x,p->y-actor.y);
            if(d<best){best=d;chosen=idx;}
        }
        if(chosen==world_.placement_count())return;
        const auto* p=world_.placement(chosen);
        if(p->object==kFurnitureBed){
            // Root event 948724703580895 chooses bed Sprite951 frame1
            // in the interaction picker; it is the SECOND frame in the
            // original bed-sheet0.png atlas, not a new artwork sheet.
            if(world_.set_placement_sprite_state(chosen,0,1,p->opacity))
                source_bed_selected_uid_=p->runtime_uid;
            // The original SleepApproved/GotoSleep hierarchy is separate:
            // do not skip days, heal or fake sleep without its full guards.
            // Bed_activated -> SleepApproved: show accurate original
            // rejection conditions; this is NOT full GotoSleep. The
            // remaining original guards/side effects must be ported first.
            const auto precheck=source_bed_sleep_precheck(
                day_night_.snapshot().timer_all,source_weather_exposure_,
                player_.state().hunger,player_.state().thirst);
            source_furniture_feedback_=source_bed_precheck_feedback(precheck);
            if(precheck==SourceBedSleepPrecheck::passed_known_conditions) {
                bool sheltered=false;
                const minidayz::WorldPoint player_pos{
                    player_.state().x,player_.state().y};
                // Root UID 145774090408998 requires collision with the
                // actual wall trigger 346 unless source perk_cn is active.
                // That perk is not yet represented, so do not invent it.
                for(const auto i:source_house_wall_indices_){
                    const auto* wall=world_.placement(i);
                    if(wall && world_.source_placement_active(*wall) && source_player_overlaps_world_sprite(
                        *wall,player_pos,wall->origin_x,wall->origin_y)){
                        sheltered=true;break;
                    }
                }
                if(!sheltered) {
                    source_furniture_feedback_="SLEEP RESTRICTED: NEED SHELTER";
                } else if(player_.state().bleeding>0.0f ||
                          player_.state().infection>0.0f) {
                    // Preserve source var22/35 medical exclusion rather
                    // than granting sleep through an unported condition.
                    source_furniture_feedback_="SLEEP RESTRICTED: WOUNDED";
                } else {
                    bool danger=false;
                    for(std::size_t n=0;n<npcs_.count();++n){
                        const auto* npc=npcs_.actor(n);
                        if(npc && npc->alive &&
                           std::hypot(npc->x-player_pos.x,
                                      npc->y-player_pos.y)<300.0f){
                            danger=true;break;
                        }
                    }
                    if(danger){
                        source_furniture_feedback_="SLEEP RESTRICTED: NEARBY THREAT";
                    }else{
                        const auto rules=preview_rules_;
                        const auto time=source_count_sleep_seconds(
                            day_night_.snapshot().timer_all,
                            player_.state().hunger,player_.state().thirst,
                            rules.hunger_drain_per_second*5.0f,
                            rules.thirst_drain_per_second*5.0f,
                            perks_.level(minidayz::items::SourcePerk::hamster),
                            perks_.level(minidayz::items::SourcePerk::camel));
                        if(time>=1.5f && std::isfinite(time)) {
                            source_sleep_={p->runtime_uid,1.0f,time};
                            source_furniture_feedback_="SLEEP APPROVED - FADING";
                        }else{
                            source_furniture_feedback_="SLEEP RESTRICTED: NO TIME";
                        }
                    }
                }
            }
            source_furniture_feedback_seconds_=3.0f;
            return;
        }
        if(p->object!=kFurniturePump)return;
        if(source_pump_pending_.runtime_uid)return;
        if(p->source_variable_0==0){
            source_furniture_feedback_="WATER PUMP EMPTY";
        }else if(player_.state().thirst>=preview_rules_.max_thirst){
            source_furniture_feedback_="THIRST ALREADY FULL";
        }else{
            const auto use=source_pump_draw(player_.state().thirst,
                p->source_variable_0,preview_rules_.max_thirst);
            if(!use.can_use)return;
            // Root 401776724287473 deducts water BEFORE the wait.
            // Root 5981791044814398 captures the remaining low volume
            // and empties the pump AFTER the wait.
            if(use.consume_before_wait && !world_.set_source_pump_water(
                    chosen,p->source_variable_0-use.consumed))return;
            source_pump_pending_={
                p->runtime_uid,static_cast<std::uint32_t>(chosen),
                kPumpUseDelay,use.thirst_gain,use.consume_before_wait};
            source_furniture_feedback_="DRINKING FROM WATER PUMP";
        }
        source_furniture_feedback_seconds_=3.0f;
    }

    void advance_source_furniture_wait(float dt) noexcept {
        if(!source_pump_pending_.runtime_uid || !std::isfinite(dt) ||
           dt<0.0f || !player_.state().alive)return;
        source_pump_pending_.seconds-=dt;
        if(source_pump_pending_.seconds>0.0f)return;
        const auto waiting=source_pump_pending_;
        source_pump_pending_={};
        const auto* p=world_.placement(waiting.index);
        if(!p || !world_.source_placement_active(*p) || p->runtime_uid!=waiting.runtime_uid ||
           !minidayz::world_source::source_furniture_active(*p) ||
           p->object!=minidayz::world_source::kFurniturePump)return;
        if(!waiting.consumed_before_wait &&
           !world_.set_source_pump_water(waiting.index,0))return;
        // The original adds the captured amount, then Update_Stats clamps
        // it; do not recalculate the drink using post-wait thirst.
        player_.adjust_thirst(waiting.thirst_gain,preview_rules_.max_thirst);
        source_furniture_feedback_="DRANK ORIGINAL PUMP WATER";
        source_furniture_feedback_seconds_=3.0f;
    }

    void update_source_furniture_bed_frame() noexcept {
        const minidayz::WorldPoint actor{
            player_.state().x,player_.state().y};
        // Root events 903835777805920 / 948724703580895 reset
        // every 951 to frame0, then highlight overlapping 951 at frame1.
        // The original does not require pressing A to show bed selection.
        source_bed_selected_uid_=0;
        for(auto idx:source_furniture_indices_){
            const auto* p=world_.placement(idx);
            if(!p || !world_.source_placement_active(*p) || p->object!=minidayz::world_source::kFurnitureBed)
                continue;
            const bool overlap=
                minidayz::world_source::source_furniture_active(*p) &&
                minidayz::world_source::source_player_overlaps_world_sprite(
                    *p,actor,p->origin_x,p->origin_y);
            const auto frame=minidayz::world_source::source_bed_overlap_frame(
                *p,overlap);
            if(p->source_frame!=frame || p->source_animation!=0)
                (void)world_.set_placement_sprite_state(idx,0,frame,p->opacity);
            if(overlap)source_bed_selected_uid_=p->runtime_uid;
        }
    }

    // Native circle-pad/D-pad cursor replaces Construct's mobile touch
    // Build-mode ghost position; all eligibility/mechanics come from root.
    bool source_bed_build_site_valid(float x,float y) const noexcept {
        using namespace minidayz::world_source;
        if(!world_ready_ || !world_.bounds().contains({x,y}) ||
           !source_bed_site_within_range(
                player_.state().x,player_.state().y,x,y))return false;
        minidayz::WorldPlacement ghost{};
        ghost.layer=41; // original "Buildings" layer
        ghost.object=kSourceBedGhostObject;
        ghost.source_frame=kSourceBedGhostFrame;
        ghost.width=kSourceBedGhostWidth;
        ghost.height=kSourceBedGhostHeight;
        ghost.origin_x=kSourceBedGhostOriginX;
        ghost.origin_y=kSourceBedGhostOriginY;
        ghost.opacity=1.0f;
        ghost.x=x;ghost.y=y;
        // Original 811103055423252 requires object573 collision overlap
        // with wall zone346. Use source per-frame collision polygons.
        // Reuse source-extracted 346 wall slots built once per world.
        // Scanning every authored Map placement each frame on a 3DS
        // would turn the original ghost preview into an O(world) stall.
        for(const auto i:source_house_wall_indices_){
            const auto* wall=world_.placement(i);
            if(wall && world_.source_placement_active(*wall) && source_house_sprites_overlap(ghost,*wall))
                return true;
        }
        return false;
    }

    bool begin_source_bed_craft(std::uint8_t variant) noexcept {
        using namespace minidayz::world_source;
        if(source_bed_build_.active ||
           source_bed_dismantle_.runtime_uid ||
           pending_craft_recipe_<minidayz::items::kSourceCraftRecipeCount ||
           !source_bed_materials_available(inventory_,variant) ||
           world_.capture_source_constructed_beds().size()>=kSourceMaxBeds)
            return false;
        const float x=player_.state().x+50.0f;
        const float y=player_.state().y;
        if(!world_.bounds().contains({x,y}))return false;
        source_bed_build_={true,false,variant,x,y,0.0f};
        crafting_open_=false;
        selected_inventory_slot_=-1;
        source_furniture_feedback_="BED GHOST: MOVE, A PLACE, B CANCEL";
        source_furniture_feedback_seconds_=3.0f;
        return true;
    }

    void advance_source_bed_build(const minidayz::InputState& input,
                                  float dt) noexcept {
        if(!source_bed_build_.active)return;
        auto& build=source_bed_build_;
        if(!build.confirmed){
            if(input.pressed & KEY_B){build={};return;}
            float mx=static_cast<float>(input.circle.dx)/156.0f;
            float my=-static_cast<float>(input.circle.dy)/156.0f;
            if(input.held & KEY_DLEFT)mx-=1.0f;
            if(input.held & KEY_DRIGHT)mx+=1.0f;
            if(input.held & KEY_DUP)my-=1.0f;
            if(input.held & KEY_DDOWN)my+=1.0f;
            const float speed=100.0f; // same native source 100 movement
            build.x+=mx*std::max(0.0f,dt)*speed;
            build.y+=my*std::max(0.0f,dt)*speed;
            if(input.pressed & KEY_A){
                if(source_bed_build_site_valid(build.x,build.y) &&
                   minidayz::world_source::source_bed_materials_available(
                       inventory_,build.variant)){
                    build.confirmed=true;
                    build.remaining=minidayz::world_source::kSourceBedBuildDelay;
                    source_furniture_feedback_="BUILDING BED - ORIGINAL 3S";
                }else source_furniture_feedback_="BED NEEDS HOUSE WALL + MATERIALS";
                source_furniture_feedback_seconds_=3.0f;
            }
            return;
        }
        build.remaining-=std::max(0.0f,dt);
        if(build.remaining>0.0f)return;
        auto inventory_after=inventory_;
        const auto x=build.x,y=build.y;
        const bool valid=source_bed_build_site_valid(x,y);
        const bool materials=
            minidayz::world_source::source_bed_consume_materials(
                inventory_after,build.variant);
        if(valid && materials &&
           world_.add_source_constructed_bed(x,y,build.variant)){
            inventory_=inventory_after;
            rebuild_source_furniture();
            visibility_index_.rebuild(world_);
            source_furniture_feedback_="BED BUILT FROM ORIGINAL MATERIALS";
            stage2_audio_.play_crafting();
        }else{
            source_furniture_feedback_="BED BUILD CANCELLED: INVALID SITE";
        }
        source_furniture_feedback_seconds_=3.0f;
        build={};
    }

    void request_source_bed_dismantle() noexcept {
        if(source_bed_dismantle_.runtime_uid)return;
        const minidayz::WorldPoint actor{player_.state().x,player_.state().y};
        std::size_t chosen=world_.placement_count();
        float distance=std::numeric_limits<float>::max();
        for(auto idx:source_furniture_indices_){
            const auto* bed=world_.placement(idx);
            if(!bed || !world_.source_placement_active(*bed) || bed->object!=minidayz::world_source::kFurnitureBed ||
               !minidayz::world_source::source_furniture_active(*bed) ||
               bed->source_variable_0>1 ||
               !minidayz::world_source::source_player_overlaps_world_sprite(
                   *bed,actor,bed->origin_x,bed->origin_y))continue;
            const float d=std::hypot(bed->x-actor.x,bed->y-actor.y);
            if(d<distance){chosen=idx;distance=d;}
        }
        if(chosen==world_.placement_count())return;
        const auto* bed=world_.placement(chosen);
        source_bed_dismantle_={bed->runtime_uid,chosen,
            minidayz::world_source::kSourceBedDismantleDelay};
        // Source UID 622329722190205 selects a random original chop
        // sample before its Wait(1), and spawns 317 at the bed hotspot.
        const auto noise=static_cast<std::uint32_t>(
            (bed->runtime_uid ^ world_seed_) & 3ULL);
        stage2_audio_.play_original_world_sound(
            static_cast<minidayz::OriginalWorldSound>(noise));
        source_furniture_feedback_="DISMANTLING BED - ORIGINAL 1S";
        source_furniture_feedback_seconds_=3.0f;
    }

    void advance_source_bed_dismantle(float dt) noexcept {
        if(!source_bed_dismantle_.runtime_uid)return;
        source_bed_dismantle_.remaining-=std::max(0.0f,dt);
        if(source_bed_dismantle_.remaining>0.0f)return;
        const auto pending=source_bed_dismantle_;
        source_bed_dismantle_={};
        const auto* bed=world_.placement(pending.index);
        if(!bed || bed->runtime_uid!=pending.runtime_uid ||
           bed->object!=minidayz::world_source::kFurnitureBed ||
           bed->opacity<=0.0f || bed->source_variable_0>1)return;
        const auto material=minidayz::world_source::source_bed_hides(
            static_cast<std::uint8_t>(bed->source_variable_0));
        // Root Check_space_inventory gives back Woodpiles50 and the
        // original Deer714 / Bear784 hide. Use ground if bag is full.
        auto after_inventory=inventory_;
        auto after_ground=ground_loot_;
        const auto add=[&](std::uint16_t id) noexcept {
            if(after_inventory.add(id,1))return true;
            return after_ground.add({id,1,
                minidayz::items::kUseSourceItemCondition,0},
                bed->x,bed->y);
        };
        if(!material || !add(50) || !add(material)){
            source_furniture_feedback_="BED RECOVERY: BAG/GROUND FULL";
        }else if(world_.dismantle_source_bed(pending.index)){
            inventory_=after_inventory;
            ground_loot_=after_ground;
            source_furniture_feedback_="BED DISMANTLED: MATERIALS RETURNED";
            rebuild_source_furniture();
            visibility_index_.rebuild(world_);
        }else source_furniture_feedback_="BED DISMANTLE CANCELLED";
        source_furniture_feedback_seconds_=3.0f;
    }

    // Root Bed_activated waits 1 original second before GotoSleep.
    // Do not advance time until the approval and source Count_sleep_hours
    // branch have both succeeded. Original 5s stat loops are discrete.
    void advance_source_sleep_wait(float dt) noexcept {
        if(!source_sleep_.runtime_uid || !std::isfinite(dt) || dt<0.0f)
            return;
        source_sleep_.remaining-=dt;
        if(source_sleep_.remaining>0.0f)return;
        const auto pending=source_sleep_;
        source_sleep_={};
        if(!player_.state().alive || !world_ready_)return;
        bool bed_valid=false;
        for(const auto index:source_furniture_indices_){
            const auto* bed=world_.placement(index);
            if(bed && world_.source_placement_active(*bed) && bed->runtime_uid==pending.runtime_uid &&
               minidayz::world_source::source_furniture_active(*bed)){
                bed_valid=true;break;
            }
        }
        if(!bed_valid)return;
        const auto steps=static_cast<unsigned>(
            std::floor(pending.sleep_seconds/5.0f));
        const auto minutes=static_cast<unsigned>(
            std::floor(pending.sleep_seconds/1.5f));
        // Source UID 985925996128270: repeat floor(sleep_seconds/5).
        for(unsigned i=0;i<steps && player_.state().alive;++i){
            (void)player_.update_survival(5.0f,preview_rules_);
            if(perks_.level(minidayz::items::SourcePerk::camel)>0)
                player_.adjust_thirst(
                    -preview_rules_.thirst_drain_per_second*5.0f*0.1f,
                    preview_rules_.max_thirst);
            if(perks_.level(minidayz::items::SourcePerk::hamster)>0)
                player_.adjust_hunger(
                    -preview_rules_.hunger_drain_per_second*5.0f*0.1f,
                    preview_rules_.max_hunger);
            const float sweethome=perks_.source_sweethome_needs_gain();
            if(sweethome>0.0f) {
                player_.adjust_hunger(sweethome,preview_rules_.max_hunger);
                player_.adjust_thirst(sweethome,preview_rules_.max_thirst);
            }
            (void)player_.apply_source_body_temperature_tick(
                source_weather_exposure_);
        }
        // Root 717349066493382: floor(sleep_seconds/1.5) time ticks.
        (void)day_night_.update(static_cast<float>(minutes)*1.5f);
        source_temperature_initialized_=false;
        stage2_audio_.set_environment_ambience(
            !day_night_.day_mode(),world_level_==4);
        source_furniture_feedback_=player_.state().alive?
            "WOKE UP - ORIGINAL SURVIVAL TICKS":
            "SLEEP ENDED - SURVIVOR DIED";
        source_furniture_feedback_seconds_=3.0f;
    }

    // Original bunker Sprite796 is a collision-triggered one-time lootbox
    // (event 389950912936355), NOT a searchable city-house cabinet.
    // Do not synthesize 796 objects for unrelated rooms: only act on
    // instances created by an original bunker event.
    void rebuild_source_interior_containers() noexcept {
        source_interior_container_indices_.clear();
        for(std::size_t i=0;i<world_.placement_count();++i){
            const auto* p=world_.placement(i);
            if(p && p->object==minidayz::world_source::kSourceBunkerLootbox &&
               p->layer==minidayz::world_source::kSourceInteriorContainerLayer)
                source_interior_container_indices_.push_back(
                    static_cast<std::uint32_t>(i));
        }
    }

    void update_source_interior_containers() noexcept {
        const minidayz::WorldPoint player_pos{
            player_.state().x,player_.state().y};
        for(const auto idx:source_interior_container_indices_){
            const auto* box=world_.placement(idx);
            if(!box || !world_.source_placement_active(*box) || !minidayz::world_source::source_bunker_lootbox_closed(*box))
                continue;
            // Root 181.OnCollision(796): use the original active-frame
            // collider and hotspot, not a guessed radius or touch button.
            if(!minidayz::world_source::source_player_overlaps_world_sprite(
                  *box,player_pos,box->origin_x,box->origin_y))continue;
            // Root 796.SetFrame(1), Fade.StartFade, var1=true. Frame1
            // is an independent source atlas rectangle in every authored
            // bunker-lootbox animation. The existing v4 world sprite
            // sidecar persists frame1 so it cannot be looted repeatedly.
            (void)world_.open_source_bunker_lootbox(idx);
            // Contents dispatch belongs to original category-specific
            // bunker loot drops, not invented storage slots. Do not grant
            // guessed items or replay open events on a saved box.
        }
    }

    void rebuild_source_automatic_doors() {
        source_door_indices_.clear();
        for(std::size_t i=0;i<world_.placement_count();++i){
            const auto* p=world_.placement(i);
            if(p && p->object==349)
                source_door_indices_.push_back(static_cast<std::uint32_t>(i));
        }
        // Rebuild when loading/generating the original source Map. This
        // vector is a per-frame transition guard, not a saved gameplay
        // state or a substitute for the original door Sprite frame.
        source_door_changed_this_tick_.assign(source_door_indices_.size(),0u);
        source_door_waits_.assign(source_door_indices_.size(),{});
    }

    void play_source_door_effect(
        minidayz::world_source::AutomaticDoorEffect effect) noexcept {
        using minidayz::world_source::AutomaticDoorEffect;
        switch(effect){
        case AutomaticDoorEffect::open_wood:
            stage2_audio_.play_original_world_sound(
                minidayz::OriginalWorldSound::doorOpen);break;
        case AutomaticDoorEffect::close_wood:
            stage2_audio_.play_original_world_sound(
                minidayz::OriginalWorldSound::doorClose);break;
        case AutomaticDoorEffect::open_metal:
            stage2_audio_.play_original_world_sound(
                minidayz::OriginalWorldSound::doorMetalOpen);break;
        case AutomaticDoorEffect::close_metal:
            stage2_audio_.play_original_world_sound(
                minidayz::OriginalWorldSound::doorMetalClose);break;
        default:break;
        }
    }

    // Mobile GUI original marker506 type 2 arrives at the selected door.
    // On 3DS, A while overlapping a door replaces mobile marker arrival;
    // it is not a reproduction of the original touchscreen pathfinding.
    bool request_source_manual_door() noexcept {
        using namespace minidayz::world_source;
        const minidayz::WorldPoint actor{player_.state().x,player_.state().y};
        std::size_t best=source_door_indices_.size();
        float distance=std::numeric_limits<float>::max();
        for(std::size_t slot=0;slot<source_door_indices_.size();++slot){
            if(slot>=source_door_waits_.size() ||
               source_door_changed_this_tick_[slot] ||
               source_door_waits_[slot].effect!=AutomaticDoorEffect::none)continue;
            const auto* p=world_.placement(source_door_indices_[slot]);
            if(!p || !world_.source_placement_active(*p) || p->source_frame>1 || !p->runtime_uid)continue;
            const auto* frame=source_door_point(p->source_animation,p->source_frame);
            if(!frame || !source_door_overlaps_player(*p,*frame,actor))continue;
            const float d=std::hypot(p->x-actor.x,p->y-actor.y);
            if(d<distance){distance=d;best=slot;}
        }
        if(best==source_door_indices_.size())return false;
        const auto index=source_door_indices_[best];
        const auto* p=world_.placement(index);
        const auto* frame=source_door_point(p->source_animation,p->source_frame);
        const bool opening=p->source_frame==0;
        const auto effect=opening ?
            (frame->metal?AutomaticDoorEffect::open_metal:AutomaticDoorEffect::open_wood):
            (frame->metal?AutomaticDoorEffect::close_metal:AutomaticDoorEffect::close_wood);
        if(opening && p->linked_runtime_uid)
            (void)world_.set_linked_door_solid_present(index,false);
        source_door_waits_[best]={effect,p->runtime_uid,0.01f,true};
        source_door_changed_this_tick_[best]=1u;
        return true;
    }

    void advance_source_door_waits(float dt) noexcept {
        if(!world_ready_ || !player_.state().alive ||
           !std::isfinite(dt))return;
        for(std::size_t slot=0;slot<source_door_waits_.size();++slot){
            auto& pending=source_door_waits_[slot];
            if(pending.effect==
                minidayz::world_source::AutomaticDoorEffect::none)continue;
            pending.seconds-=std::max(0.0f,dt);
            if(pending.seconds>0.0f)continue;
            const auto effect=pending.effect;
            const auto uid=pending.runtime_uid;
            const bool manual=pending.manual;
            pending={};
            if(slot>=source_door_indices_.size())continue;
            const auto index=source_door_indices_[slot];
            const auto* active_door=world_.placement(index);
            if(!active_door || !world_.source_placement_active(*active_door))continue;
            // A door's pending event always belongs to THAT generated
            // instance, even after loading a saved world or moving away.
            using minidayz::world_source::AutomaticDoorEffect;
            const bool opening=effect==AutomaticDoorEffect::open_wood ||
                               effect==AutomaticDoorEffect::open_metal;
            const bool applied=manual ?
                minidayz::world_source::apply_verified_door_frame_at_index(
                    world_,index,uid,opening?7258332154939593ULL:353730214514351ULL):
                minidayz::world_source::source_apply_auto_door_intention(
                    world_,index,uid,effect);
            if(applied){
                source_door_changed_this_tick_[slot]=1u;
                play_source_door_effect(effect);
            }else{
                // A failed/stale pending event must not leave an original
                // closed 349 with its Solid 348 already destroyed. Restore
                // only its own original collider from the live door frame.
                if(const auto* door=world_.placement(index);
                   door && door->runtime_uid==uid)
                    (void)world_.set_linked_door_solid_present(
                        index,door->source_frame==0u);
            }
        }
    }

    void update_source_automatic_doors() noexcept {
        if(!world_ready_ || !player_.state().alive)return;
        const minidayz::WorldPoint player_pos{
            player_.state().x,player_.state().y
        };
        for(std::size_t slot=0;slot<source_door_indices_.size();++slot){
            if(slot<source_door_changed_this_tick_.size() &&
               source_door_changed_this_tick_[slot])continue;
            if(slot<source_door_waits_.size() &&
               source_door_waits_[slot].effect!=
                   minidayz::world_source::AutomaticDoorEffect::none)
                continue;
            const auto index=source_door_indices_[slot];
            const auto* p=world_.placement(index);
            if(!p || !world_.source_placement_active(*p))continue;
            // Check the real source door dimensions and hotspot, not a
            // guessed 96px range that can suppress large doors and their
            // original wood/metal open/close sound events.
            if(!minidayz::world_source::source_sprite_might_touch_player(
                    p->x,p->y,p->width,p->height,p->scale_x,
                    player_pos.x,player_pos.y))continue;
            const auto event=
                minidayz::world_source::source_auto_door_intended_for_placement(
                    world_,index,player_pos);
            using minidayz::world_source::AutomaticDoorEffect;
            if(event!=AutomaticDoorEffect::none &&
               slot<source_door_waits_.size()){
                // Open source UID 6061166023614368 DESTROYS the
                // linked Solid 348 BEFORE System Wait(0.01), allowing
                // the player into the building immediately. After the
                // wait it selects door sprite frame 1 and plays its WAV.
                // Close UID 243854400993751 waits first; its SetFrame0
                // and linked Solid-348 Spawn happen on commit.
                const bool opening=
                    event==AutomaticDoorEffect::open_wood ||
                    event==AutomaticDoorEffect::open_metal;
                if(opening && p->linked_runtime_uid!=0)
                    (void)world_.set_linked_door_solid_present(index,false);
                source_door_waits_[slot]={event,p->runtime_uid,0.01f};
                source_door_changed_this_tick_[slot]=1u;
            }
        }
    }

    void start_next_source_map() noexcept {
        if (world_level_ >= 5) return;
        const auto destination_level = static_cast<std::uint8_t>(world_level_ + 1);
        auto destination_seed = world_seed_ ^
            (0x9e3779b9u + static_cast<std::uint32_t>(destination_level) * 0x85ebca6bu);
        if (destination_seed == 0) destination_seed = 0x6d696e69u;

        // Keep the active world and its save state usable until the
        // destination has generated and accepted the survivor's spawn.
        if (!travel_.begin(destination_seed, destination_level, 0, true, true, true, true, true, true, true)) {
            travel_failed_ = true;
            return;
        }
        travel_failed_ = false;
        loading_world_ = true;
    }

    void fail_world_generation(bool traveling = false) noexcept {
        loading_world_ = false;
        if (traveling) {
            travel_.cancel();
            travel_failed_ = true;
            return;
        }
        world_ready_ = false;
        generation_failed_ = true;
        menu_active_ = true;
        menu_page_ = minidayz::menu::BootPage::main;
        menu_selection_ = minidayz::menu::default_selection(has_saved_game_);
        boot_input_gate_.reset();
    }

    void start_new_game() noexcept {
        continue_world_visual_state_=false;
        world_seed_ = make_new_world_seed();
        stage2_audio_.stop_environment_ambience();
        stage2_audio_.stop_rain_loop();
        footstep_elapsed_=0.0f;
        world_level_ = 0;
        legacy_world_generation_ = false;
        legacy_split_roads_ = false;
        legacy_crash_coordinates_ = false;
        legacy_site_generation_ = false;
        legacy_start_order_ = false;
        legacy_category3_events_ = false;
        legacy_category5_events_ = false;
        legacy_category46_events_ = false;
        legacy_category7_events_ = false;
        legacy_exact_crash_selection_ = false;
        legacy_active_location_events_ = false;
        player_ = minidayz::PlayerSystem(make_initial_state());
        player_.set_source_skin(
            minidayz::stage1::kOriginalPlayerSkins[selected_skin_].object);
        inventory_.clear();
        equipment_.clear();
        ground_loot_.clear();
        tents_.clear();
        stashes_.clear();
        day_night_.reset();
        twilight_fade_.reset();
        rain_timer_.reset();
        loot_timer_.reset();
        item_use_runtime_.clear();
        source_dead_animation_elapsed_=0.0f;
        source_energy_drink_.clear();
        perks_.clear();
        source_temperature_initialized_ = false;
        source_weather_exposure_ = 0.0f;
        source_body_tick_elapsed_ = 0.0f;
        source_furniture_indices_.clear();
        source_pump_pending_={};
        source_bed_selected_uid_=0;
        source_furniture_feedback_="";
        source_furniture_feedback_seconds_=0.0f;
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
    // CurrentLevel>4 overrides with UID 641409757696071:
    // (random(1000,16500),373).
    // Never retry, snap to a road, or shift the position on collision:
    // those extra rules changed the original starting position.
    bool place_new_player_on_generated_map(minidayz::WorldSystem& destination,
                                           minidayz::PlayerState& state) noexcept {
        const auto& bounds=destination.bounds();
        if(!(bounds.width>0.0f && bounds.height>0.0f))return false;
        minidayz::WorldPoint start{};
        if (destination.source_start_position(start)) {
            if (!bounds.contains(start)) return false;
            state.x=start.x;
            state.y=start.y;
            return true; // no additional random draw after loot generation
        }
        // Compatibility for a caller using the pre-v27 native pipeline.
        // Continue restores its saved XY and never calls this fallback.
        auto& source_rng=destination.source_random();
        const double unit=static_cast<double>(source_rng())/4294967296.0;
        const bool snow=destination.generation_level()==4u; // legacy native rule
        const double range=std::max(0.0,
            static_cast<double>(bounds.height)-
            minidayz::stage1::kNormalPlayerStartLayoutMargin);
        const float px=snow?
            static_cast<float>(minidayz::stage1::kSnowPlayerStartXMin+
                unit*(minidayz::stage1::kSnowPlayerStartXMax-
                      minidayz::stage1::kSnowPlayerStartXMin))
            :minidayz::stage1::kNormalPlayerStartX;
        const float py=snow?minidayz::stage1::kSnowPlayerStartY:
            static_cast<float>(
                minidayz::stage1::kNormalPlayerStartYBase+unit*range);
        if(!std::isfinite(px)||!std::isfinite(py))return false;
        state.x=px;
        state.y=py;
        return true;
    }

    void begin_world_generation() noexcept {
        generation_failed_ = false;
        travel_failed_ = false;
        world_ready_ = false;
        npc_ready_ = false;
        menu_active_ = true;
        source_door_indices_.clear();
        source_door_changed_this_tick_.clear();
        source_door_waits_.clear();
        source_roof_indices_.clear();
        source_house_interior_indices_.clear();
        source_house_wall_indices_.clear();
        source_overlapping_wall_indices_.clear();
        source_foundation_tint_.clear();
        source_player_outline_=false;
        source_roof_elapsed_=0.0f;
        source_roof_background_layer_=0xffff;
        loading_world_ = world_.begin_source_generation(world_seed_, "Map", world_level_, 0, legacy_world_generation_, legacy_split_roads_, legacy_crash_coordinates_, legacy_site_generation_, !legacy_start_order_, !legacy_category3_events_, !legacy_category5_events_, !legacy_category46_events_, !legacy_category7_events_, !legacy_exact_crash_selection_, !legacy_active_location_events_);
        if (!loading_world_) fail_world_generation();
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

    void play_source_footsteps(float x,float y) noexcept {
        using minidayz::terrain::FootstepSurface;
        bool indoors=false, wood=false, concrete=false;
        for(const auto index:source_house_wall_indices_){
            const auto* zone=world_.placement(index);
            if(!zone || !world_.source_placement_active(*zone) || !minidayz::world_source::source_player_base_overlaps_wall(*zone,{x,y}))
                continue;
            indoors=true;
            wood |= zone->source_variable_1==1;
            concrete |= zone->source_variable_1==2;
        }
        // Each material is a separate picked-instance subevent of overlap 346.
        if(wood)stage2_audio_.play_footstep(FootstepSurface::wood,world_.source_random()());
        if(concrete)stage2_audio_.play_footstep(FootstepSurface::concrete,world_.source_random()());
        if(!indoors)stage2_audio_.play_footstep(terrain_surface_at(x,y),world_.source_random()());
    }

    minidayz::terrain::FootstepSurface terrain_surface_at(
        float x,float y) const noexcept {
        if(world_.source_pond_contains(x,y,false))
            return minidayz::terrain::FootstepSurface::shallow_water;
        const auto surface=terrain_collision_.surface_at(x,y);
        if (surface!=minidayz::terrain::FootstepSurface::ground)
            return surface;
        const auto& grid=world_.grid();
        const auto& bounds=world_.bounds();
        if(grid.width()==0||grid.height()==0||
           bounds.width<=0.0f||bounds.height<=0.0f||x<0.0f||y<0.0f||
           x>=bounds.width||y>=bounds.height)
            return surface;
        if(world_.source_ground_frame_at(x,y)==15)
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
        for(const auto& solid:scene->world_.source_bridge_solids()){
            bool known=false;
            if(minidayz::world_source::source_building_polygon_contains(
                    solid,{x,y},known))return true;
            if(!known && minidayz::terrain::source_solid_sprite_contains(
                    solid.x,solid.y,solid.width,solid.height,
                    solid.origin_x,solid.origin_y,solid.angle,x,y,solid.scale_x))
                return true;
        }
        if (scene->terrain_collision_.blocked(x,y) ||
            scene->world_.source_pond_contains(x,y,true)) return true;
        // Original deployed tent foundation Sprite398 has Solid behavior.
        // It lives in the survivor tent store, not the static world index.
        // Root frame[10] is a concave U-shaped wall with an open entrance.
        // Use the source mask in pixel coordinates relative to its hotspot
        // (25,33), leaving the interior walkable; never block the roof PNG.
        for(std::size_t i=0;i<scene->tents_.tent_count();++i){
            const auto* tent=scene->tents_.tent(i);
            if(!tent)continue;
            const auto* frame=minidayz::stage1::source_building_frame_geometry(398,0,0);
            if(!frame)continue;
            minidayz::WorldPlacement solid{};
            solid.object=398;solid.x=tent->x;solid.y=tent->y;
            solid.width=frame->width;solid.height=frame->height;
            solid.origin_x=frame->origin_x;solid.origin_y=frame->origin_y;
            solid.scale_x=1.0f;
            if(minidayz::terrain::source_world_solid_mask_contains(solid,x,y))return true;
        }
        for (const auto i : scene->collision_index_.candidates(x,y)) {
            const auto* placement = scene->world_.placement(i);
            // A tilemap's rectangular bounds mostly contain EMPTY cells.
            // Only actual original Solid cells (above) are obstacles.
            if (placement) {
                const auto* kind= minidayz::stage1::find_object(placement->object);
                if (kind && kind->type==16) continue;
            }
            if (!placement || !scene->world_.source_placement_active(*placement) ||
                placement->width <= 0.0f || placement->height <= 0.0f ||
                minidayz::terrain::source_visual_roof_overlay(
                    placement->object)) continue;
            const auto* layer = minidayz::stage1::find_layer(
                map_index, placement->layer);
            if (!layer) continue;
            const bool building = minidayz::terrain::source_building_collision_layer(
                layer->name,placement->object);
            // Construct sprite collision is behavior-based. Visible
            // building floors/decor must not become opaque solid walls.
            // A Solid may live on any source layer. The tree fallback
            // below is the only canopy exception; paired trees use their
            // actual source companion, including its changed stump frame.
            // Opening a source door removes the linked Solid object
            // from collision/picking and drawing. A parked native sprite
            // with opacity 0 preserves its stable original UID linkage.
            if(placement->object==348 &&
               placement->linked_runtime_uid!=0 &&
               placement->opacity<=0.0f)continue;
            if(placement->object==349){
                // A linked original Solid 348 owns this doorway's actual
                // collision. Don't test its decorative door sheet twice.
                if(placement->linked_runtime_uid!=0)continue;
                // Source fallback for an unpaired/unmapped original door.
                if(!building || placement->source_frame!=0)continue;
                bool source_collider_known=false;
                const bool hit=
                    minidayz::world_source::original_closed_door_solid_contains(
                        *placement,{x,y},source_collider_known);
                if(hit)return true;
                if(source_collider_known)continue;
                // Unmapped original animations must remain safely closed
                // rather than silently creating a walk-through wall.
                if(minidayz::terrain::source_solid_sprite_contains(
                        placement->x,placement->y,
                        placement->width,placement->height,
                        placement->origin_x,placement->origin_y,
                        placement->angle,x,y,placement->scale_x))
                    return true;
                continue;
            }
            // Original tree trunks are linked invisible Sprite345 on
            // zed_resp. Keep player collision consistent with the world
            // broadphase: ordinary zombie/loot respawn markers stay nonsolid.
            // Grounded tree trunk fallback for unlinked/authored originals.
            // Test the original Sprite345 Imagepoint1 footprint, not the
            // entire visible tree canopy. The linked 345 companion still
            // participates in collision independently.
            if (std::strcmp(layer->name,"Trees")==0 &&
                (minidayz::world_source::source_tree_impact_point(
                    placement->object)!=nullptr ||
                 placement->object==210)) {
                if(placement->linked_runtime_uid==0 &&
                   (minidayz::terrain::source_standing_tree_trunk_contains(
                        *placement,x,y) ||
                   minidayz::terrain::source_tree_block_solid_contains(
                        *placement,x,y)))return true;
                continue;
            }
            if(!minidayz::stage1::source_object_has_solid_behavior(
                   placement->object))continue;
            if(minidayz::terrain::source_world_solid_mask_contains(
                   *placement,x,y))return true;
        }
        return false;
    }

    minidayz::WorldSystem world_{};
    minidayz::WorldSpecialTerrain menu_source_terrain_{};
    minidayz::terrain::SolidTerrain terrain_collision_{};
    minidayz::terrain::SourceObstacleIndex collision_index_{};
    struct SourceDoorWait {
        minidayz::world_source::AutomaticDoorEffect effect{
            minidayz::world_source::AutomaticDoorEffect::none};
        std::uint64_t runtime_uid{};
        float seconds{};
        bool manual{};
    };
    struct SourcePumpWait {
        std::uint64_t runtime_uid{};
        std::uint32_t index{};
        float seconds{};
        float thirst_gain{};
        bool consumed_before_wait{};
    };
    struct SourceBedBuild {
        bool active{};
        bool confirmed{};
        std::uint8_t variant{};
        float x{},y{},remaining{};
    };
    struct SourceBedDismantle {
        std::uint64_t runtime_uid{};
        std::size_t index{};
        float remaining{};
    };
    SourceBedBuild source_bed_build_{};
    SourceBedDismantle source_bed_dismantle_{};
    struct SourceSleepWait {
        std::uint64_t runtime_uid{};
        float remaining{};
        float sleep_seconds{};
    };
    SourceSleepWait source_sleep_{};
    std::vector<std::uint32_t> source_furniture_indices_{};
    std::vector<std::uint32_t> source_interior_container_indices_{};
    SourcePumpWait source_pump_pending_{};
    std::uint64_t source_bed_selected_uid_{};
    const char* source_furniture_feedback_="";
    float source_furniture_feedback_seconds_{};
    std::vector<std::uint32_t> source_door_indices_{};
    std::vector<std::uint8_t> source_door_changed_this_tick_{};
    std::vector<SourceDoorWait> source_door_waits_{};
    std::vector<std::uint32_t> source_roof_indices_{};
    std::vector<std::uint32_t> source_house_interior_indices_{};
    std::vector<std::uint32_t> source_house_wall_indices_{};
    std::vector<std::uint32_t> source_house_foundation_indices_{};
    std::vector<std::uint32_t> source_overlapping_wall_indices_{};
    std::vector<std::uint8_t> source_foundation_tint_{};
    bool source_player_outline_{};
    float source_roof_elapsed_{};
    float source_interior_elapsed_{};
    std::uint16_t source_roof_buildings_layer_{0xffff};
    std::uint16_t source_roof_base_layer_{0xffff};
    std::uint16_t source_roof_base_top_layer_{0xffff};
    std::uint16_t source_roof_background_layer_{0xffff};
    minidayz::terrain::VisibleSourcePlacementIndex visibility_index_{};
    std::vector<std::uint32_t> visible_world_placements_{};
    minidayz::NpcSystem npcs_{};
    std::uint32_t world_seed_{};
    bool legacy_world_generation_{};
    bool legacy_split_roads_{};
    bool legacy_crash_coordinates_{};
    bool legacy_site_generation_{};
    bool legacy_start_order_{};
    bool legacy_category3_events_{};
    bool legacy_category5_events_{};
    bool legacy_category46_events_{};
    bool legacy_category7_events_{};
    bool legacy_exact_crash_selection_{};
    bool legacy_active_location_events_{};
    bool source_location_creation_retry_{};
    std::size_t source_location_creation_retry_count_{};
    std::uint8_t world_level_{};
    minidayz::WorldTravel travel_;
    bool travel_failed_{};
    bool continue_world_visual_state_{};
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
    minidayz::items::SourceTwilightFade twilight_fade_{};
    minidayz::items::SourceRainTimer rain_timer_{};
    minidayz::items::SourceLootTimer loot_timer_{};
    minidayz::items::ItemUseRuntime item_use_runtime_{};
    minidayz::stage1::SourceEnergyDrinkBoost source_energy_drink_{};
    float source_dead_animation_elapsed_{};
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
    minidayz::map_overlay::CellKind map_selected_kind_{
        minidayz::map_overlay::CellKind::ground};
};

} // namespace

namespace {
// Native startup diagnostics only; do not change original game events or saves.
void boot_checkpoint(const char* stage, bool reset) noexcept {
    (void)mkdir("sdmc:/3ds",0777);
    (void)mkdir("sdmc:/3ds/Minidayz",0777);
    if(auto* file=std::fopen("sdmc:/3ds/Minidayz/boot.log",reset?"w":"a")){
        std::fprintf(file,"%s\n",stage);
        std::fclose(file);
    }
}
void show_boot_failure(const char* stage) noexcept {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,nullptr);
    std::printf("Mini DAYZ 3DS startup failed\n%s\nPress START to exit.\n",stage);
    while(aptMainLoop()){
        hidScanInput();
        if(hidKeysDown()&KEY_START)break;
        gspWaitForVBlank();
    }
    gfxExit();
}
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    boot_checkpoint("main entered",true);
    // The expanded 128-page cache made Engine roughly 13 KiB larger.
    // Keep the renderer and its cache metadata off the limited main stack.
    static minidayz::Engine engine;
    if (!engine.initialize()) {
        boot_checkpoint("engine initialization failed");
        show_boot_failure("Engine / RomFS / GPU initialization");
        return 1;
    }
    boot_checkpoint("engine initialized");
    {
        auto& renderer=engine.renderer();
        renderer.begin_frame();
        renderer.begin_screen(GFX_TOP);
        renderer.draw_text("MINI DAYZ - STARTING",40.0f,100.0f,0.6f);
        renderer.begin_screen(GFX_BOTTOM);
        renderer.draw_text("LOADING MENU",20.0f,100.0f,0.45f);
        renderer.end_frame();
    }

    std::unique_ptr<SavedGame> restored(new(std::nothrow) SavedGame{});
    if(!restored){
        boot_checkpoint("save buffer allocation failed");
        engine.shutdown();
        show_boot_failure("Save buffer allocation");
        return 1;
    }
    boot_checkpoint("reading save");
    const bool has_saved_game = load_saved_game(*restored);
    boot_checkpoint(has_saved_game?"save validated":"no valid save");
    std::unique_ptr<WorldPreviewScene> scene(new(std::nothrow)
        WorldPreviewScene(has_saved_game ? restored.get() : nullptr));
    if (!scene) {
        boot_checkpoint("scene allocation failed");
        engine.shutdown();
        show_boot_failure("Scene allocation");
        return 1;
    }
    restored.reset();
    boot_checkpoint("attaching scene");
    if (!engine.set_scene(scene.get())) {
        boot_checkpoint("scene attachment failed");
        engine.shutdown();
        show_boot_failure("Scene attachment");
        return 1;
    }
    boot_checkpoint("entering main loop");
    engine.run();
    engine.shutdown();
    return 0;
}
