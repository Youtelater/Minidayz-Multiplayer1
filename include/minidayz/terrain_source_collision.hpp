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
    // Citro2D rotates around its sprite's top-left draw point. Offset by
    // source hotspot before the inverse rotation.
    const float left=hot_x-width*origin_x;
    const float top=hot_y-height*origin_y;
    float px=point_x-left,py=point_y-top;
    if(angle_degrees!=0.0f){
        constexpr float radians=0.01745329251994329577f;
        const float angle=angle_degrees*radians;
        const float sn=std::sin(angle),cs=std::cos(angle);
        const float xx=px*cs+py*sn;
        const float yy=-px*sn+py*cs;
        px=xx;py=yy;
    }
    return px>=0.0f&&py>=0.0f&&px<=width&&py<=height;
}
}
