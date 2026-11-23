#pragma once
namespace minidayz::stage1 {
// Root Construct project[3][193] PlayerBase sprite frame0:
// width2/height2, hotspot (1,1). Map PlayerBase instance UID924
// is ALSO exactly 2x2, so its anchored collision footprint is
// [playerX-2, playerX] x [playerY-2, playerY].
// Sprite 679 visible artwork is 30x30 and does NOT define collisions.
inline constexpr float kSourcePlayerBaseHalfWidth=1.0f;
inline constexpr float kSourcePlayerBaseHalfHeight=1.0f;
inline constexpr float kSourcePlayerBaseCollisionOffsetX=-1.0f;
inline constexpr float kSourcePlayerBaseCollisionOffsetY=-1.0f;
} // namespace minidayz::stage1
