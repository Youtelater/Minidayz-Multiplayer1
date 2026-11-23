#pragma once
#include <cstdint>

namespace minidayz::world_source {

// Root original Construct object IDs and canonical assets/gfx mappings.
// These are *gameplay/event instances*, not scenery. Keep every source
// placement, source UID, source frame, mask, collision and saved world state.
// Only the automatic top-screen world sprite pass must skip their PIXELS.
// 181 player_collision_base; 193 player_base; 218 spawn_loot_point;
// 345 obst_base (red collision silhouettes); 346 warmzone (wall trigger);
// 348 big_obstacle_base (neon green hitbox silhouettes);
// 404 ach_city_counter (1000x1000 magenta region overlay);
// 508 map_element; 514 mapgen; 857 polygon_target_base;
// 984 collision_checker_general. All mapped to canonical root source frames.
inline constexpr std::uint16_t kSourceNonvisualCityCounter=404;
constexpr bool source_world_logic_marker_not_art(
    std::uint16_t object) noexcept {
    switch(object){
    case 181: case 193: case 218:
    case 345: case 346: case 348:
    case 344: case 434:
    case 404: case 508: case 514:
    case 857: case 984:
        return true;
    default:
        return false;
    }
}

// Source-exact exceptions to the top world-scene cutoff before browser
// overlays and GUI. The original category6 create_map_element event emits
// REAL Sprite241 car-lootbox art onto "Sunset_Dawn" (Map layer45), not
// Buildings/vehicle layers. Previous global layer>=44 culling hid ALL of
// those original overlays even though source spawn states were present.
// Root generator tools/stage1/build_source_car_wreck_trunks.py verifies
// the original Sprite241 layer and imagepoint from data.deobfuscated.js.
// Sprite1126 is original world radiation art on Rain layer44.
constexpr bool source_world_high_layer_gameplay_art(
    std::uint16_t object,std::uint16_t layer) noexcept {
    return (object==241 && layer==45) ||
           (object==1126 && layer==44);
}

// build_ghost-sheet0 (571) and build_ghost_obstacle-sheet0/1 (573)
// are *real* source animation frames, but only drawn under the original
// GUI/Building mode event conditions. Their Map layout instances are staged
// outside the playfield at opacity 0. A native, active construction preview
// is explicitly drawn in WorldPreviewScene::render(), never by this generic
// WorldPlacement pass. Do NOT remove the original PNGs or their source atlas.
constexpr bool source_world_preview_needs_live_event(
    std::uint16_t object) noexcept {
    return object==571 || object==573;
}
} // namespace minidayz::world_source
