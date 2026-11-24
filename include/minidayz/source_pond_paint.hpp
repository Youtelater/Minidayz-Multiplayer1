#pragma once
#include "minidayz/source_water_shore_effective_tile.hpp"
#include <vector>
namespace minidayz::world_source {
// Original marker3 event 5104880853224267 creates Tilemap208 at
// Mapgen514.X/Y minus 500, size 1050x1050; clear 17x17.
// Event 9368941097190071 paints three ranges; Repeat 225 scans 15x15.
inline constexpr int kPondSide=17;
inline constexpr int kPondScanSide=15;
inline constexpr float kPondOffset=500.0f;
inline constexpr float kPondTileSize=60.0f;
inline std::vector<std::uint32_t> source_pond_cells() {
    std::vector<std::uint32_t> cells(kPondSide*kPondSide,terrain::kTileEmpty);
    const auto paint=[&](int x,int y,int w,int h){
        for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i)
            cells[j*kPondSide+i]=4;
    };
    paint(4,4,8,8);
    paint(3,5,10,5);
    paint(6,3,4,10);
    for(int y=0;y<kPondScanSide;++y)
        for(int x=0;x<kPondScanSide;++x)
            cells[y*kPondSide+x]=terrain::source_water_shore_effective_tile(
                cells,kPondSide,kPondSide,x,y);
    return cells;
}
}
