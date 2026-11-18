#pragma once
#include <algorithm>
#include <cmath>

namespace minidayz::terrain {

// Conservative source-sprite culling: Construct places sprites at a hotspot,
// which may be hundreds of pixels outside the 400x240 top-screen viewport.
// A fixed 96px origin margin discards large original houses, forests, and
// bridges while their art is still plainly visible.
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
        !std::isfinite(angle_radians)) return false;
    // Rotation about the source hotspot is safely enclosed by the maximum
    // radius to any of the original image's four transformed corners.
    const float x0=-origin_x*frame_width*scale_x;
    const float x1=(1.0f-origin_x)*frame_width*scale_x;
    const float y0=-origin_y*frame_height*scale_y;
    const float y1=(1.0f-origin_y)*frame_height*scale_y;
    const float l=std::min(x0,x1),r=std::max(x0,x1);
    const float t=std::min(y0,y1),b=std::max(y0,y1);
    if (angle_radians != 0.0f) {
        const float radius=std::hypot(std::max(std::fabs(l),std::fabs(r)),
                                      std::max(std::fabs(t),std::fabs(b)));
        return hotspot_x+radius>camera_x &&
               hotspot_x-radius<camera_x+view_width &&
               hotspot_y+radius>camera_y &&
               hotspot_y-radius<camera_y+view_height;
    }
    return hotspot_x+r>camera_x && hotspot_x+l<camera_x+view_width &&
           hotspot_y+b>camera_y && hotspot_y+t<camera_y+view_height;
}

} // namespace minidayz::terrain
