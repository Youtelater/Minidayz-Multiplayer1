#pragma once
#include "minidayz/source_mapgen_geometry.hpp"
#include <array>
#include <cstdint>
#include <cmath>

namespace minidayz::world_source {

// Root Game_events create_map_location road categories 12/13/14 each
// execute the same 12 ordered SetTile leaves inside Repeat(289).
// The selected ground-family1066 tilemap is picked by Sprite514.var6 UID.
// Compare neighbors to original tile15 (not simply a nonempty tile).
// Bit index=(dy+1)*3+dx+1; bit4 is the center, checked by the parent.
// Source conditions and ordering are independently checked in
// tests/test_original_road_paint_geometry.py against root data.
struct SourceRoadEdgeRule { std::uint8_t tile; std::uint16_t equal, unequal; };
inline constexpr SourceRoadEdgeRule kSourceRoadEdgeRules[] = {
    {11,128,47},
    {5,8,422},
    {1,2,488},
    {7,32,203},
    {3,256,239},
    {4,64,431},
    {9,1,494},
    {8,4,491},
    {0,11,416},
    {2,38,200},
    {10,200,38},
    {12,416,11},
};
inline constexpr std::uint8_t kUnpaintedRoadTile=255;
inline constexpr int kSourceRoadPatchSide=17;
inline constexpr int kSourceRoadPatchCells=289;

constexpr std::uint8_t source_road_edge_frame(
    std::uint8_t center,std::uint16_t neighbors) noexcept {
    if(center==15)return kUnpaintedRoadTile;
    std::uint8_t result=kUnpaintedRoadTile;
    // Original separate sibling events execute in their authored order.
    for(const auto& rule:kSourceRoadEdgeRules)
        if((neighbors&rule.equal)==rule.equal &&
           (neighbors&rule.unequal)==0)
            result=rule.tile;
    return result;
}

struct SourceRoadPaintPatch {
    std::array<std::uint8_t,kSourceRoadPatchCells> cells{};
    std::uint8_t at(int x,int y) const noexcept {
        if(x<0||y<0||x>=kSourceRoadPatchSide||y>=kSourceRoadPatchSide)
            return kUnpaintedRoadTile;
        return cells[static_cast<std::size_t>(y)*kSourceRoadPatchSide+x];
    }
};

template<class Grid>
SourceRoadPaintPatch source_grid_road_paint_patch(
    const Grid& grid,int col,int row) noexcept {
    SourceRoadPaintPatch patch{};
    patch.cells.fill(kUnpaintedRoadTile);
    // This patch covers the recovered code12/13/14 branches only.
    // The other location types have their own independent paint events.
    const auto code=grid.at(col,row);
    if(code!=12&&code!=13&&code!=14)return patch;
    constexpr float tile=60.0f;
    constexpr float width=source_road_painted_width(tile);
    const auto center=mapgen_hotspot(col,row);
    const float origin_x=center.x-8.5f*tile;
    const float origin_y=center.y-8.5f*tile;
    // Apply the original source-backed range geometry first, then the
    // 17x17 local scan. Neighbor reads outside this modeled picked
    // tilemap are non-tile15; the full instance-picking executor remains
    // a separate parity gate.
    for(int segment=0;segment<5;++segment){
        const auto band=source_grid_road_segment(grid,col,row,segment,width);
        if(!band.valid())continue;
        const int x0=static_cast<int>(std::lround((band.x-origin_x)/tile));
        const int y0=static_cast<int>(std::lround((band.y-origin_y)/tile));
        const int nx=static_cast<int>(std::lround(band.width/tile));
        const int ny=static_cast<int>(std::lround(band.height/tile));
        for(int y=y0;y<y0+ny;++y)for(int x=x0;x<x0+nx;++x)
            if(x>=0&&y>=0&&x<kSourceRoadPatchSide&&y<kSourceRoadPatchSide)
                patch.cells[static_cast<std::size_t>(y)*kSourceRoadPatchSide+x]=15;
    }
    for(int y=0;y<kSourceRoadPatchSide;++y)
        for(int x=0;x<kSourceRoadPatchSide;++x){
            if(patch.at(x,y)==15)continue;
            std::uint16_t neighbors=0;
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
                if(patch.at(x+dx,y+dy)==15)
                    neighbors|=static_cast<std::uint16_t>(1u<<((dy+1)*3+dx+1));
            const auto frame=source_road_edge_frame(patch.at(x,y),neighbors);
            if(frame!=kUnpaintedRoadTile)
                patch.cells[static_cast<std::size_t>(y)*kSourceRoadPatchSide+x]=frame;
        }
    return patch;
}

} // namespace minidayz::world_source
