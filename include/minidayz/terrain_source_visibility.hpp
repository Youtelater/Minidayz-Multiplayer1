#pragma once
#include <algorithm>
#include <cmath>

namespace minidayz::terrain {

// Citro2D negative scales flip UVs but do NOT move the geometry. Keep the
// original Construct hotspot in the same world position when mirrored.
inline float source_sprite_top_left(float hotspot,float size,
                                    float origin,float scale) noexcept {
    const float pixel_extent=std::fabs(scale)*size;
    return hotspot-(scale<0.0f ? 1.0f-origin : origin)*pixel_extent;
}

// Camera test uses the same top-left and rotation semantics as Citro2D.
// Construct world sprites are sometimes far larger than 96px, so culling
// their hotspot alone incorrectly hides visible forests and buildings.
inline bool source_sprite_in_view(
    float hotspot_x, float hotspot_y, float frame_width, float frame_height,
    float origin_x, float origin_y, float scale_x, float scale_y,
    float angle_radians, float camera_x, float camera_y,
    float view_width, float view_height) noexcept {
    if (!(frame_width > 0.0f && frame_height > 0.0f) ||
        !(view_width > 0.0f && view_height > 0.0f))
        return false;
    if (!std::isfinite(hotspot_x) || !std::isfinite(hotspot_y) ||
        !std::isfinite(scale_x) || !std::isfinite(scale_y) ||
        !std::isfinite(angle_radians) ||
        !std::isfinite(origin_x) || !std::isfinite(origin_y)) return false;

    const float x=source_sprite_top_left(hotspot_x,frame_width,origin_x,scale_x);
    const float y=source_sprite_top_left(hotspot_y,frame_height,origin_y,scale_y);
    const float w=std::fabs(scale_x)*frame_width;
    const float h=std::fabs(scale_y)*frame_height;
    if (angle_radians == 0.0f) {
        return x+w>camera_x && x<camera_x+view_width &&
               y+h>camera_y && y<camera_y+view_height;
    }
    // Citro2D rotates the quad about its draw origin by default, not about
    // the sprite's hotspot: include every rotated corner in the test.
    const float sn=std::sin(angle_radians),cs=std::cos(angle_radians);
    const float x1=x+w*cs, y1=y+w*sn;
    const float x2=x-h*sn, y2=y+h*cs;
    const float x3=x+w*cs-h*sn, y3=y+w*sn+h*cs;
    const float left=std::min(std::min(x,x1),std::min(x2,x3));
    const float right=std::max(std::max(x,x1),std::max(x2,x3));
    const float top=std::min(std::min(y,y1),std::min(y2,y3));
    const float bottom=std::max(std::max(y,y1),std::max(y2,y3));
    return right>camera_x && left<camera_x+view_width &&
           bottom>camera_y && top<camera_y+view_height;
}

} // namespace minidayz::terrain
