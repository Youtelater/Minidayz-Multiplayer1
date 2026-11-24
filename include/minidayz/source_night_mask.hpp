#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace minidayz::world_source {

// Original isolated layer destination-out: multiply destination alpha by
// (1 - source alpha * instance opacity). The uniform overlay RGB survives.
inline std::uint8_t source_night_destination_out(std::uint8_t dest,
    std::uint8_t src,float opacity) noexcept {
    const auto erase=static_cast<unsigned>(std::lround(src*std::clamp(opacity,0.0f,1.0f)));
    return static_cast<std::uint8_t>((dest*(255u-erase)+127u)/255u);
}

// Rasterize an original Sprite frame with its signed width, hotspot and
// rotation. Pixel-center sampling matches nearest pixel-art filtering.
inline void source_night_apply_light(std::uint8_t* mask,unsigned sw,unsigned sh,
    const std::uint8_t* source,unsigned fw,unsigned fh,
    float x,float y,float width,float height,float ox,float oy,
    float angle,bool mirror,float opacity) noexcept {
    if(!mask||!source||!fw||!fh||!sw||!sh||!(width>0&&height>0&&opacity>0))return;
    const float co=std::cos(angle),si=std::sin(angle);
    const float mx=mirror?-1.0f:1.0f;
    const float radius=std::hypot(width*std::max(std::fabs(ox),std::fabs(1-ox)),
                                 height*std::max(std::fabs(oy),std::fabs(1-oy)));
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(radius)||
       !std::isfinite(co)||!std::isfinite(si)||
       x+radius<=0||y+radius<=0||x-radius>=sw||y-radius>=sh)return;
    const auto limit=[](float v,unsigned n){
        return static_cast<unsigned>(std::clamp(v,0.0f,static_cast<float>(n)));
    };
    const auto x0=limit(std::floor(x-radius),sw),x1=limit(std::ceil(x+radius),sw);
    const auto y0=limit(std::floor(y-radius),sh),y1=limit(std::ceil(y+radius),sh);
    for(unsigned row=y0;row<y1;++row)for(unsigned col=x0;col<x1;++col){
        const float dx=col+0.5f-x,dy=row+0.5f-y;
        const float u=(dx*co+dy*si)*mx/width+ox;
        const float v=(-dx*si+dy*co)/height+oy;
        if(u<0||v<0||u>=1||v>=1)continue;
        const auto a=source[std::min(static_cast<unsigned>(v*fh),fh-1)*fw+
                            std::min(static_cast<unsigned>(u*fw),fw-1)];
        auto& dest=mask[row*sw+col];
        dest=source_night_destination_out(dest,a,opacity);
    }
}

// PICA200 textures are row-major 8x8 tiles, each Morton-interleaved.
// Texture coordinates for an unrotated subtexture use top=1, bottom<1.
inline std::size_t source_night_tiled_index(unsigned x,unsigned y,unsigned stride) noexcept {
    unsigned m=0;
    for(unsigned bit=0;bit<3;++bit){
        m|=((x>>bit)&1u)<<(2u*bit);
        m|=((y>>bit)&1u)<<(2u*bit+1u);
    }
    return (static_cast<std::size_t>(y/8)*(stride/8)+x/8)*64+m;
}
} // namespace minidayz::world_source
