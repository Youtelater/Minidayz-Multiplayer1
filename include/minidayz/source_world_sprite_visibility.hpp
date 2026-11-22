#pragma once
#include <cstdint>

namespace minidayz::world_source {

// Original Construct t404 / ach_city_counter-sheet0.png is a 2x2
// MAGENTA gameplay counter marker. Source Map generation creates
// 1000x1000 instances on Buildings layer 41 to track achievement areas.
// These are not scenery: rendering their pixels covers entire towns and
// hides the player in a purple square. Preserve WorldPlacement state,
// original location and UID for game events; skip only the pixel draw.
// Intentionally do not hide other IDs without matching root-source proof.
inline constexpr std::uint16_t kSourceNonvisualCityCounter=404;
constexpr bool source_world_logic_marker_not_art(
    std::uint16_t object) noexcept {
    return object==kSourceNonvisualCityCounter;
}

} // namespace minidayz::world_source
