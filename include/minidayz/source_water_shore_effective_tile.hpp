#pragma once
#include "minidayz/generated/water_shore_source.hpp"
#include "minidayz/terrain_source_tile_flags.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidayz::terrain {

// Root Construct Game_events: twelve original Tilemap208 SetTile
// coastline branches. Their predicates query the TARGET tile as well as
// its neighbors. The previously invented EMPTY-only filter skipped the
// original frame4 water tiles, so neither the renderer nor collision ever
// applied those authored branches. Evaluate them against the same map
// cells, in original source action order, for BOTH rendering and Solid.
// The missing procedural source SetTile passes remain a separate gate.
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
    // Preserve authored terrain unless this is source water frame4,
    // or an empty target whose exact original checks allow new paint.
    // A shoreline SetTile frame replaces the previous orientation;
    // do not reuse the original tile's flip flags without a source action.
    if(authored!=kTileEmpty && source_tile_frame(authored)!=4u)
        return authored;

    for(const auto& rule:stage1::kWaterShoreRules) {
        // Ignore zero-positive rules until the original procedural
        // water-creation loop and its selection scope are implemented:
        // otherwise an all-empty Map could acquire invented water tiles.
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
    // No matching original SetTile event means no write. In
    // particular, the interior source water tile4 must not disappear.
    return authored;
}
} // namespace minidayz::terrain
