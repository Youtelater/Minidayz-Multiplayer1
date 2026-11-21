#pragma once
#include <cmath>

namespace minidayz::world_source {

// Broadphase for player interactions with original Construct world sprites.
// The source sprite hotspot may be anywhere inside a rotated image, and
// buildings/doors can be much larger than the provisional 96/256px checks.
// The farthest possible corner is never beyond hypot(full width,full height)
// from the hotspot. Add the original 30x30 player sprite's half diagonal.
// WorldPlacement width/height already store the full rendered extent;
// its scale_x field is a mirror sign, NOT another size multiplier.
// This is a conservative REJECTION check: actual picking/overlap must still
// be resolved by the original source-specific narrow-phase handler.
// A restored 25% roof must always be rechecked so it can return to 100%.
inline bool source_sprite_might_touch_player(
    float sprite_x, float sprite_y, float width, float height,
    float scale_x, float player_x, float player_y,
    bool force_recheck = false) noexcept {
    if (!std::isfinite(sprite_x) || !std::isfinite(sprite_y) ||
        !std::isfinite(player_x) || !std::isfinite(player_y) ||
        !std::isfinite(width) || !std::isfinite(height) ||
        !std::isfinite(scale_x) || width <= 0.0f || height <= 0.0f)
        return false;
    if (force_recheck) return true;
    constexpr float kOriginalPlayerHalfDiagonal = 21.213204f; // 30x30
    const float radius =
        std::hypot(width,height) +
        kOriginalPlayerHalfDiagonal;
    return std::hypot(player_x-sprite_x,player_y-sprite_y) <= radius;
}
} // namespace minidayz::world_source
