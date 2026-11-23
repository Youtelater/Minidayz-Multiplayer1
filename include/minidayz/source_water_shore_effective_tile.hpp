#pragma once
#include "minidayz/generated/water_shore_source.hpp"
#include "minidayz/terrain_source_tile_flags.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidayz::terrain {

// Root Construct Game_events: twelve Tilemap208 SetTile coastline branches.
// The renderer already painted these source frames in EMPTY cells beside
// the original authored water-frame4 neighbors. The original port had a
// split-brain bug: the renderer displayed a shoreline while the collision
// and footstep code used only the EMPTY pre-paint RLE cell.
//
// This function is the read-only effective tile for BOTH systems. It
// retains the exact source rule order, predicates and literal tile IDs;
// zero-neighbor branches are not applied without their missing original
// procedural water-creation event.
inline std::uint32_t source_water_shore_effective_tile(
    const std::vector<std::uint32_t>& cells,
    std::uint32_t columns,std::uint32_t rows,
    int column,int row) noexcept {
    if(column<0 || row<0 ||
       static_cast<std::uint32_t>(column)>=columns ||
       static_cast<std::uint32_t>(row)>=rows ||
       !columns || !rows ||
       cells.size()<static_cast<std::size_t>(columns)*rows)
        return kTileEmpty;
    const auto index=static_cast<std::size_t>(row)*columns+
                     static_cast<std::size_t>(column);
    const auto authored=cells[index];
    if(authored!=kTileEmpty)return authored;

    for(const auto& rule:stage1::kWaterShoreRules) {
        bool has_original_water=false;
        for(std::uint16_t i=0;i<rule.count;++i)
            if(stage1::kWaterShoreChecks[rule.first+i].requires_water)
                has_original_water=true;
        if(!has_original_water)continue;
        bool match=true;
        for(std::uint16_t i=0;i<rule.count;++i) {
            const auto& check=stage1::kWaterShoreChecks[rule.first+i];
            const auto nx=column+check.dx,ny=row+check.dy;
            const bool neighbor=nx>=0 && ny>=0 &&
                static_cast<std::uint32_t>(nx)<columns &&
                static_cast<std::uint32_t>(ny)<rows &&
                cells[static_cast<std::size_t>(ny)*columns+
                      static_cast<std::size_t>(nx)]!=kTileEmpty &&
                source_tile_frame(cells[
                    static_cast<std::size_t>(ny)*columns+
                    static_cast<std::size_t>(nx)])==4u;
            if(neighbor!=check.requires_water){match=false;break;}
        }
        if(match)return rule.frame;
    }
    return kTileEmpty;
}
} // namespace minidayz::terrain
