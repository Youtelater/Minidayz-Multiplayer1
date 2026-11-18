#pragma once
#include <cstdint>

namespace minidayz::terrain {

// Construct/Tiled tilemap cell format: the three highest bits encode
// horizontal, vertical and anti-diagonal flips. Preserve them when packing
// original Map RLE into 3DS atlas descriptors, rather than silently losing
// rotations of shoreline, road and forest tiles.
inline constexpr std::uint32_t kTileFlipH=0x80000000u;
inline constexpr std::uint32_t kTileFlipV=0x40000000u;
inline constexpr std::uint32_t kTileFlipD=0x20000000u;
inline constexpr std::uint32_t kTileFlipMask=0xe0000000u;
inline constexpr std::uint32_t kTileEmpty=0xffffffffu;
inline constexpr std::uint32_t kTileFrameMask=0x0000ffffu;
inline constexpr std::uint32_t kTilePageMask=0x1fff0000u;

struct SourceTileTransform {
    float scale_x,scale_y,angle_radians;
};
constexpr std::uint32_t source_tile_frame(std::uint32_t descriptor) noexcept {
    return descriptor & kTileFrameMask;
}
constexpr std::uint32_t source_tile_page(std::uint32_t descriptor) noexcept {
    return (descriptor & kTilePageMask)>>16;
}
constexpr std::uint32_t source_tile_flags(std::uint32_t descriptor) noexcept {
    return descriptor & kTileFlipMask;
}
constexpr std::uint32_t pack_source_tile(
    std::uint32_t flags,std::uint32_t page,std::uint32_t frame) noexcept {
    return (flags & kTileFlipMask) | ((page & 0x1fffu)<<16) |
           (frame & kTileFrameMask);
}
// Diagonal flip (anti-diagonal in the source sheet) is applied before
// horizontal and vertical flips. Rotate the original square tile 90 degrees
// clockwise and choose the equivalent UV reflection signs.
constexpr SourceTileTransform source_tile_transform(
    std::uint32_t descriptor) noexcept {
    const bool h=(descriptor & kTileFlipH)!=0;
    const bool v=(descriptor & kTileFlipV)!=0;
    const bool d=(descriptor & kTileFlipD)!=0;
    if(d) return {v?1.0f:-1.0f,h?-1.0f:1.0f,1.5707963267948966192f};
    return {h?-1.0f:1.0f,v?-1.0f:1.0f,0.0f};
}

} // namespace minidayz::terrain
