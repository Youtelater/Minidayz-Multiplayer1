#include "minidayz/source_road_uv_crop.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
using minidayz::world_source::SourceRoadTileCrop;
using minidayz::world_source::source_road_tile_crop;
static bool close(float a,float b) { return std::fabs(a-b)<0.00001f; }

int main() {
    SourceRoadTileCrop c{};
    // A full original 60px road tile has unchanged texture coordinates
    // and draws at source size. A partial 30x15 terminal tile keeps the
    // SOURCE texels, not a scaled copy of all 60x60 road pixels.
    assert(source_road_tile_crop(0.1f,0.9f,0.9f,0.1f,
                                60,60,60.0f,60.0f,c));
    assert(close(c.right,0.9f)&&close(c.bottom,0.1f));
    assert(c.width==60&&c.height==60);
    assert(close(c.scale_x,1.0f)&&close(c.scale_y,1.0f));

    assert(source_road_tile_crop(0.1f,0.9f,0.9f,0.1f,
                                60,60,30.0f,15.0f,c));
    assert(close(c.right,0.5f)&&close(c.bottom,0.7f));
    assert(c.width==30&&c.height==15);
    assert(close(c.scale_x,1.0f)&&close(c.scale_y,1.0f));

    // Tex3DS rotates some atlas entries: top < bottom means quarter
    // counter-clockwise. Cropping must preserve that orientation while
    // cutting the SAME visible part of the original source image.
    assert(source_road_tile_crop(0.1f,0.1f,0.9f,0.9f,
                                60,60,30.0f,15.0f,c));
    assert(close(c.right,0.5f)&&close(c.bottom,0.3f));
    assert(c.width==30&&c.height==15);

    // Fractional world geometry stays fractional even though Tex3DS
    // subtexture dimensions are integer pixel counts.
    assert(source_road_tile_crop(0.0f,1.0f,1.0f,0.0f,
                                60,60,2.5f,0.5f,c));
    assert(c.width==3&&c.height==1);
    assert(close(c.scale_x,2.5f/3.0f)&&close(c.scale_y,0.5f));
    assert(close(c.right,2.5f/60.0f));
    assert(close(c.bottom,1.0f-0.5f/60.0f));

    // Invalid or non-finite clipping geometry cannot submit atlas pixels.
    assert(!source_road_tile_crop(0,1,1,0,0,60,20,20,c));
    assert(!source_road_tile_crop(0,1,1,0,60,60,0,20,c));
    assert(!source_road_tile_crop(0,1,1,0,60,60,-1,20,c));
    assert(!source_road_tile_crop(0,1,1,0,60,60,
                                  std::numeric_limits<float>::quiet_NaN(),20,c));
    assert(!source_road_tile_crop(0,1,1,0,60,60,
                                  std::numeric_limits<float>::infinity(),20,c));
    std::puts("PASS pixel-preserving original source road UV crop");
}
