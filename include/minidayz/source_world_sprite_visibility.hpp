#pragma once
#include "minidayz/generated/world_source_data.hpp"
#include <cstdint>
#include <cstring>

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
// REAL Sprite241 car-lootbox art onto "Sunset_Dawn" (original Sunset_Dawn Map layer), not
// Buildings/vehicle layers. Previous global layer>=44 culling hid ALL of
// those original overlays even though source spawn states were present.
// Root generator tools/stage1/build_source_car_wreck_trunks.py verifies
// the original Sprite241 layer and imagepoint from data.deobfuscated.js.
// Sprite1126 is original world radiation art on Rain layer44.
inline bool source_world_high_layer_gameplay_art(
    std::uint16_t object,const char* source_layer_name,
    std::uint64_t source_creation_event_uid=0) noexcept {
    // Source Map layer indices can change when the authoritative root
    // Construct export regenerates stage1_data.hpp. Select by the actual
    // original layer NAME, not by a stale cached numeric index.
    if (!source_layer_name) return false;
    if ((object==241 && std::strcmp(source_layer_name,"Sunset_Dawn")==0) ||
        (object==1126 && std::strcmp(source_layer_name,"Rain")==0))
        return true;
    // Exact category5 Sprite439 -> Sprite663 night light creation actions.
    if(object==663 && std::strcmp(source_layer_name,"night")==0 &&
       (source_creation_event_uid==341262524995903ULL ||
        source_creation_event_uid==5644133325728841ULL ||
        source_creation_event_uid==5580795606807771ULL))return true;

    // The original create_map_element source also creates world sprites
    // on high event layers, e.g. object1417/1419 on "night" and 1420 on
    // "Rain". An old two-object allowlist silently hid those *original*
    // images even though their Sprite and frame were generated correctly.
    // Admit only an exact object + layer + source creation-event match from
    // ROOT data.deobfuscated.js; do NOT expose unrelated browser UI,
    // weather templates, hidden logic markers or unverified sprites.
    if (!source_creation_event_uid ||
        source_world_logic_marker_not_art(object))
        return false;
    for (const auto& original : stage1::kWorldElementSpawnSource)
        if (original.uid==source_creation_event_uid &&
            original.object==object &&
            std::strcmp(original.layer_name,source_layer_name)==0)
            return true;
    return false;
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
