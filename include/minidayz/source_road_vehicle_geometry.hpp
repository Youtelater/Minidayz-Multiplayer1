#pragma once
#include <cstdint>

namespace minidayz::world_source {
// Root Game_events functions Car_respawn_uaz / volga / hammer / ka,
// called AFTER generate_helicrashes by UID1131020651387518.
// Original Construct frame0 sizes/hotspots, not whole PNG-sheet guesses.
struct OriginalRoadVehicleFrame {
    std::uint16_t object{},overlay{};
    float width{},height{},origin_x{},origin_y{};
    float overlay_width{},overlay_height{},overlay_origin_x{},overlay_origin_y{};
};
inline constexpr OriginalRoadVehicleFrame kRoadVehicleUaz{
    809,1175,72.0f,32.0f,0.5f,0.40625f,
    86.0f,53.0f,0.43023255f,0.54716980f
};
inline constexpr OriginalRoadVehicleFrame kRoadVehicleVolga{
    815,1177,89.0f,35.0f,0.50561798f,0.40000001f,
    98.0f,49.0f,0.45918366f,0.51020408f
};
inline constexpr OriginalRoadVehicleFrame kRoadVehicleHammer{
    920,0,72.0f,32.0f,0.5f,0.40625f,0,0,0,0
};
inline constexpr OriginalRoadVehicleFrame kRoadVehicleKa{
    1343,0,91.0f,41.0f,0.5f,0.40625f,0,0,0,0
};
} // namespace minidayz::world_source
