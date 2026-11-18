#include "minidayz/terrain_source_tile_flags.hpp"
#include <cassert>
#include <cmath>

int main() {
    using namespace minidayz::terrain;
    constexpr auto h=pack_source_tile(kTileFlipH,1,4);
    constexpr auto v=pack_source_tile(kTileFlipV,0,3);
    constexpr auto d=pack_source_tile(kTileFlipD,5,12);
    static_assert(source_tile_page(h)==1 && source_tile_frame(h)==4);
    static_assert(source_tile_page(v)==0 && source_tile_frame(v)==3);
    static_assert(source_tile_page(d)==5 && source_tile_frame(d)==12);
    static_assert(source_tile_flags(h)==kTileFlipH);
    static_assert(source_tile_flags(v)==kTileFlipV);
    static_assert(source_tile_flags(d)==kTileFlipD);
    constexpr auto left=source_tile_transform(kTileFlipH);
    constexpr auto up=source_tile_transform(kTileFlipV);
    constexpr auto rotated=source_tile_transform(kTileFlipD);
    constexpr auto diagonal_horizontal=source_tile_transform(kTileFlipD|kTileFlipH);
    constexpr auto diagonal_vertical=source_tile_transform(kTileFlipD|kTileFlipV);
    static_assert(left.scale_x==-1 && left.scale_y==1);
    static_assert(up.scale_x==1 && up.scale_y==-1);
    static_assert(rotated.scale_x==-1 && rotated.scale_y==1);
    static_assert(diagonal_horizontal.scale_x==-1 && diagonal_horizontal.scale_y==-1);
    static_assert(diagonal_vertical.scale_x==1 && diagonal_vertical.scale_y==1);
    assert(std::fabs(rotated.angle_radians-1.5707963f)<0.00001f);
    assert(source_tile_frame(kTileEmpty)==65535u);
    // Both ground and environment descriptors keep source flip flags while
    // using distinct original PNG frames and source atlas pages.
    for (unsigned flags=0;flags<8;++flags) {
        const auto transform=source_tile_transform(
            pack_source_tile(flags<<29,7,127));
        assert((transform.scale_x==1 || transform.scale_x==-1) &&
               (transform.scale_y==1 || transform.scale_y==-1));
    }
}
