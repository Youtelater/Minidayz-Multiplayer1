#pragma once
#include <cstdint>
#include <initializer_list>
#include "minidayz/terrain_roads.hpp"
namespace minidayz::world_source {
// Root original Game_events UID 3669795404796534 creates Sprite514 on
// zed_resp, X=700+int(loopindex/Map_size_X)*1020,
// Y=320+(loopindex%Map_size_X)*1020. ArrayX=mod, ArrayY=quotient.
inline constexpr float kMapgenStartX=700.0f;
inline constexpr float kMapgenStartY=320.0f;
inline constexpr float kMapgenStride=1020.0f;
// Game_events / Location loader root SetTileRange actions
// 358622687629146 (514 code12) and 532767105758898 (code13)
// paint tile15 across 17x2 or 2x17 original 60px tiles.
// The old 3DS renderer incorrectly made the road only ONE tile wide.
inline constexpr int kOriginalRoadLengthTiles=17;
inline constexpr int kOriginalRoadWidthTiles=2;
constexpr float source_road_painted_width(float tile_pixels) noexcept {
 return tile_pixels*static_cast<float>(kOriginalRoadWidthTiles);
}

inline constexpr std::uint64_t kMapgenCreateEvent=3669795404796534ULL;
struct MapgenHotspot {float x{},y{};};
constexpr MapgenHotspot mapgen_hotspot(int array_x,int array_y) noexcept {
 return {kMapgenStartX+kMapgenStride*array_y,
         kMapgenStartY+kMapgenStride*array_x};
}
// Source horizontal Array roads become physically vertical (world Y)
// because Sprite514's array axes are transposed on the Map layout.
// A source code14 cell means both road axes occupy the same world-space
// 60px junction. Draw the Array-horizontal (world-vertical) band there;
 // suppress the duplicate tile from the perpendicular band. Otherwise the
// two different source-atlas UVs depth-fight at the same Construct Map
// asphalt Z and visibly flicker while the player walks past.
constexpr bool mapgen_duplicate_crossing_tile(
    std::uint16_t code,bool array_horizontal,int array_x,int array_y,
    float tile_world_x,float tile_width,float road_width) noexcept {
 if(code!=14 || array_horizontal || !(tile_width>0.0f) ||
    !(road_width>0.0f))return false;
 const auto center=mapgen_hotspot(array_x,array_y);
 constexpr float epsilon=0.01f;
 return tile_world_x>=center.x-road_width*0.5f-epsilon &&
        tile_world_x+tile_width<=center.x+road_width*0.5f+epsilon;
}

constexpr terrain::RoadBand mapgen_road_band(
 std::uint16_t code,bool array_horizontal,int array_x,int array_y,
 float road_width) noexcept {
 if(!(road_width>0.0f) || (array_horizontal
    ? !terrain::horizontal_road(code) : !terrain::vertical_road(code)))return {};
 const auto c=mapgen_hotspot(array_x,array_y);
 return array_horizontal
   ? terrain::RoadBand{c.x-road_width*0.5f,c.y-kMapgenStride*0.5f,
                       road_width,kMapgenStride}
   : terrain::RoadBand{c.x-kMapgenStride*0.5f,c.y-road_width*0.5f,
                       kMapgenStride,road_width};
}

struct MapgenCell { int column, row; };

inline MapgenCell mapgen_cell_at(float x,float y) noexcept {
 return {static_cast<int>(std::floor((y-kMapgenStartY)/kMapgenStride+0.5f)),
         static_cast<int>(std::floor((x-kMapgenStartX)/kMapgenStride+0.5f))};
}

inline bool point_on_mapgen_road(std::uint16_t code,MapgenCell cell,
                                 float x,float y,float width) noexcept {
 for(bool horizontal:{true,false}){
  const auto band=mapgen_road_band(code,horizontal,cell.column,cell.row,width);
  if(x>=band.x && x<band.x+band.width && y>=band.y && y<band.y+band.height)
   return true;
 }
 return false;
}
}
