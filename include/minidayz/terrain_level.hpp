#pragma once
#include <cstdint>

namespace minidayz::terrain {

// Recovered from root data.deobfuscated.js (Construct Map generation events).
// CurrentLevel 0..4 select objects 482, 687, 688, 776, 980.
// The final source map uses object 1139 (ground_lvl7); object 1138
// (ground_lvl6) is a distinct later-world/second-half ground variant.
// Keep this table separate from draw order; sprite sheet index is not level.
inline constexpr std::uint32_t kGroundObjectByTexture[7] = {
    482, 687, 688, 776, 980, 1138, 1139
};

constexpr std::uint8_t active_ground_texture(std::uint8_t level) noexcept {
    return level <= 4 ? level : 6;
}

// The last original 33-row world has a separate, source-authored
// second-half terrain image. Its WorldGenerationProfile starts the lower
// half at grid row 18; use that world-space boundary when drawing.
constexpr std::uint8_t ground_texture_at(std::uint8_t level,
                                         float world_y,
                                         float second_half_y) noexcept {
    return level==5 && world_y>=second_half_y ? 5
                                             : active_ground_texture(level);
}

constexpr std::uint32_t active_ground_object(std::uint8_t level) noexcept {
    return kGroundObjectByTexture[active_ground_texture(level)];
}

} // namespace minidayz::terrain
