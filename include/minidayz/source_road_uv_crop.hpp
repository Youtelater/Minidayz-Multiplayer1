#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace minidayz::world_source {

// Citro2D/Tex3DS-compatible partial original road-tile crop, without
// Citro2D-dependent code. UVs are in the original loaded t3x atlas.
// For atlas-rotated subtextures, top < bottom; interpolating left->right
// and top->bottom still follows the physical source-image x/y axes.
struct SourceRoadTileCrop {
    float right{};
    float bottom{};
    std::uint16_t width{};
    std::uint16_t height{};
    float scale_x{};
    float scale_y{};
};

inline bool source_road_tile_crop(
    float left,float top,float right,float bottom,
    std::uint16_t source_width,std::uint16_t source_height,
    float visible_width,float visible_height,
    SourceRoadTileCrop& cropped) noexcept {
    cropped={};
    if(!source_width||!source_height ||
       !std::isfinite(left)||!std::isfinite(top)||
       !std::isfinite(right)||!std::isfinite(bottom)||
       !std::isfinite(visible_width)||!std::isfinite(visible_height)||
       visible_width<=0.0f||visible_height<=0.0f)return false;
    const float w=static_cast<float>(source_width);
    const float h=static_cast<float>(source_height);
    const float ratio_x=std::clamp(visible_width/w,0.0f,1.0f);
    const float ratio_y=std::clamp(visible_height/h,0.0f,1.0f);
    cropped.right=left+(right-left)*ratio_x;
    cropped.bottom=top+(bottom-top)*ratio_y;
    cropped.width=static_cast<std::uint16_t>(
        std::max(1.0f,std::ceil(w*ratio_x)));
    cropped.height=static_cast<std::uint16_t>(
        std::max(1.0f,std::ceil(h*ratio_y)));
    cropped.scale_x=visible_width/static_cast<float>(cropped.width);
    cropped.scale_y=visible_height/static_cast<float>(cropped.height);
    return std::isfinite(cropped.right)&&std::isfinite(cropped.bottom)&&
           std::isfinite(cropped.scale_x)&&std::isfinite(cropped.scale_y);
}

} // namespace minidayz::world_source
