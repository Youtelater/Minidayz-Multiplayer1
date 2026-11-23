#pragma once
#include "minidayz/generated/source_solid_tile_polygons.hpp"
#include "minidayz/terrain_source_tile_flags.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace minidayz::terrain {

// The Construct Tilemap Solid behavior uses the per-TILE polygon of the
// original object[13] data, not a solid rectangle for every painted cell.
// Work in normalized, local source-tile coordinates. The 3DS render
// descriptor preserves Tiled H/V/anti-diagonal flips; collision must undo
// those exact transforms before testing the original authored polygon.
//
// Missing polygon records retain the previous full-tile collision fallback.
// This avoids inventing a non-solid hole for a tile with no custom polygon.
inline bool source_solid_tile_contains(
    std::uint16_t object,std::uint32_t descriptor,
    float unit_x,float unit_y) noexcept {
    if(descriptor==kTileEmpty || !std::isfinite(unit_x) ||
       !std::isfinite(unit_y) ||
       unit_x<0.0f || unit_y<0.0f ||
       unit_x>=1.0f || unit_y>=1.0f) return false;

    // Undo the original tile UV transform in reverse order. Flip D is
    // reflection across the anti-diagonal, before source H/V flips.
    if(descriptor & kTileFlipH)unit_x=1.0f-unit_x;
    if(descriptor & kTileFlipV)unit_y=1.0f-unit_y;
    if(descriptor & kTileFlipD){
        const float previous_x=unit_x;
        unit_x=1.0f-unit_y;
        unit_y=1.0f-previous_x;
    }

    const std::uint32_t tile_id=source_tile_frame(descriptor);
    const auto* first=kOriginalSolidTilePolygons;
    const auto* last=first+kOriginalSolidTilePolygonCount;
    const auto* poly=std::lower_bound(first,last,
        std::pair<std::uint16_t,std::uint32_t>{object,tile_id},
        [](const OriginalSolidTilePolygon& candidate,
           const std::pair<std::uint16_t,std::uint32_t>& wanted) {
            return std::pair<std::uint16_t,std::uint32_t>{
                candidate.object,candidate.tile}<wanted;
        });
    if(poly==last || poly->object!=object || poly->tile!=tile_id ||
       poly->count<3)return true; // no custom polygon: full source tile

    // Crossings + boundary hit: the browser's Solid overlaps are inclusive
    // of their collision edges. Keep slope/shoreline gaps walkable.
    bool inside=false;
    constexpr float epsilon=0.00001f;
    for(std::uint16_t i=0,j=poly->count-1;i<poly->count;j=i++){
        const auto& a=kOriginalSolidTileVertices[poly->first+j];
        const auto& b=kOriginalSolidTileVertices[poly->first+i];
        const float cross=(unit_x-a.x)*(b.y-a.y)-
                          (unit_y-a.y)*(b.x-a.x);
        const float dot=(unit_x-a.x)*(unit_x-b.x)+
                        (unit_y-a.y)*(unit_y-b.y);
        if(std::fabs(cross)<=epsilon && dot<=epsilon)return true;
        if((a.y>unit_y)!=(b.y>unit_y)){
            const float intersection=
                a.x+(unit_y-a.y)*(b.x-a.x)/(b.y-a.y);
            if(unit_x<intersection)inside=!inside;
        }
    }
    return inside;
}
} // namespace minidayz::terrain
