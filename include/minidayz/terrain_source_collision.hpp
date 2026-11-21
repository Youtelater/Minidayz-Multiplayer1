#pragma once
#include <cmath>

namespace minidayz::terrain {

// Original Construct world placements store a hotspot, source dimensions,
// and an angle in degrees. Evaluate the true rotated rectangle rather than
// its unrotated bounding box (which blocks doorways beside slanted walls).
inline bool source_solid_sprite_contains(
    float hot_x,float hot_y,float width,float height,
    float origin_x,float origin_y,float angle_degrees,
    float point_x,float point_y) noexcept {
    if (!(width>0.0f&&height>0.0f) ||
        !std::isfinite(hot_x)||!std::isfinite(hot_y)||
        !std::isfinite(origin_x)||!std::isfinite(origin_y)||
        !std::isfinite(angle_degrees)||!std::isfinite(point_x)||
        !std::isfinite(point_y)) return false;
    // Construct rotates the rectangle AROUND its image hotspot, not
    // around the top-left corner. Previously the inverse transform was
    // applied after a top-left translation, which moved rotated house
    // collision rectangles away from their real walls and doorways.
    // Convert the query to hotspot-relative coordinates FIRST, undo the
    // rotation, then add the original frame's hotspot offset.
    const float dx=point_x-hot_x,dy=point_y-hot_y;
    float local_x=dx,local_y=dy;
    if(angle_degrees!=0.0f){
        constexpr float radians=0.01745329251994329577f;
        const float angle=angle_degrees*radians;
        const float sn=std::sin(angle),cs=std::cos(angle);
        local_x=dx*cs+dy*sn;
        local_y=-dx*sn+dy*cs;
    }
    const float px=local_x+width*origin_x;
    const float py=local_y+height*origin_y;
    return px>=0.0f&&py>=0.0f&&px<=width&&py<=height;
}
}
