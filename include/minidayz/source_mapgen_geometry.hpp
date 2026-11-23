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
 // Original 17-cell patch starts at the cell anchor minus 8.5
 // tiles. Its two-cell road occupies indices 8 and 9, hence it begins
 // half a tile BEFORE the Mapgen hotspot and ends 1.5 tiles after it.
 // With the source-authored two-tile road_width this is -width/4,
 // not an invented symmetric -width/2 strip.
 const float start=center.x-road_width*0.25f;
 return tile_world_x>=start-epsilon &&
        tile_world_x+tile_width<=start+road_width+epsilon;
}

constexpr terrain::RoadBand mapgen_road_band(
 std::uint16_t code,bool array_horizontal,int array_x,int array_y,
 float road_width) noexcept {
 if(!(road_width>0.0f) || (array_horizontal
    ? !terrain::horizontal_road(code) : !terrain::vertical_road(code)))return {};
 const auto c=mapgen_hotspot(array_x,array_y);
 // Original family1066 paint: 17x2 / 2x17 cells, with the
 // 2-cell transverse strip beginning at tile index8 of the 17-tile
 // Mapgen location. The strip center is HALF a tile past the marker,
 // not centered exactly on the marker as the old native renderer did.
 return array_horizontal
   ? terrain::RoadBand{c.x-road_width*0.25f,c.y-kMapgenStride*0.5f,
                       road_width,kMapgenStride}
   : terrain::RoadBand{c.x-kMapgenStride*0.5f,c.y-road_width*0.25f,
                       kMapgenStride,road_width};
}

// Original Game_events Location loader / Sprite514 code14
// UID 1239724375006415 does NOT paint two unconditional 17-cell
// corridors. It executes four separate family1066.SetTileRange(202)
// commands, each guarded by an OR-block of ten Array518 neighbor codes.
// Source actions (in source order):
// 1586657255052358: Array(col-1,row), x+8,y,   size2x9
// 8618616698851008: Array(col+1,row), x+8,y+8, size2x9
// 4462790001866082: Array(col,row-1), x,y+8,   size9x2
// 7247777744760893: Array(col,row+1), x+8,y+8, size9x2.
// Sprite514.var2 is array column; .var1 is array row (confirmed by
// SourceLocationEventRuntime::set_mapgen). Preserve the original 10
// literals, including 103; do not turn this into "any nonzero tile."
constexpr bool source_road_neighbor_permits_junction(std::uint16_t code) noexcept {
 switch(code){
 case 9: case 6: case 4: case 12: case 13:
 case 14: case 5: case 11: case 15: case 103: return true;
 default:return false;
 }
}
template<class Grid>
inline terrain::RoadBand source_grid_road_segment(
    const Grid& grid,int array_col,int array_row,int segment,
    float road_width) noexcept {
 if(!(road_width>0.0f) || segment<0 || segment>=4)return {};
 const auto code=grid.at(array_col,array_row);
 // The root code12 / code13 base SetTileRange actions are 17x2 / 2x17.
 if(code==12) return segment==0?
     mapgen_road_band(code,false,array_col,array_row,road_width):
     terrain::RoadBand{};
 if(code==13) return segment==0?
     mapgen_road_band(code,true,array_col,array_row,road_width):
     terrain::RoadBand{};
 if(code!=14)return {};
 const std::uint16_t neighbor=
     segment==0?grid.at(array_col-1,array_row):
     segment==1?grid.at(array_col+1,array_row):
     segment==2?grid.at(array_col,array_row-1):
                grid.at(array_col,array_row+1);
 if(!source_road_neighbor_permits_junction(neighbor))return {};
 const auto c=mapgen_hotspot(array_col,array_row);
 const float tile=road_width/static_cast<float>(kOriginalRoadWidthTiles);
 const float beginning_x=c.x-8.5f*tile;
 const float beginning_y=c.y-8.5f*tile;
 const float half_length=9.0f*tile;
 switch(segment){
 case 0: return {beginning_x+8.0f*tile,beginning_y,
                  road_width,half_length};
 case 1: return {beginning_x+8.0f*tile,beginning_y+8.0f*tile,
                  road_width,half_length};
 case 2: return {beginning_x,beginning_y+8.0f*tile,
                  half_length,road_width};
 default:return {beginning_x+8.0f*tile,beginning_y+8.0f*tile,
                  half_length,road_width};
 }
}
// Two valid source actions can repaint the same central road tile.
// In the 3DS rasterizer equal-depth overlapping submissions flicker
// while the player walks. Draw each original tile once in source order.
template<class Grid>
inline bool source_grid_road_tile_was_drawn(
    const Grid& grid,int array_col,int array_row,int segment,
    float tile_x,float tile_y,float tile_size,float road_width) noexcept {
 if(segment<=0 || !(tile_size>0.0f))return false;
 for(int earlier=0;earlier<segment;++earlier){
  const auto band=source_grid_road_segment(
      grid,array_col,array_row,earlier,road_width);
  if(!band.valid())continue;
  constexpr float epsilon=0.01f;
  if(tile_x>=band.x-epsilon && tile_y>=band.y-epsilon &&
     tile_x+tile_size<=band.x+band.width+epsilon &&
     tile_y+tile_size<=band.y+band.height+epsilon)return true;
 }
 return false;
}
template<class Grid>
inline bool source_grid_road_contains(
    const Grid& grid,int array_col,int array_row,
    float x,float y,float road_width) noexcept {
 for(int segment=0;segment<4;++segment){
  const auto band=source_grid_road_segment(
      grid,array_col,array_row,segment,road_width);
  if(band.valid() && x>=band.x && x<band.x+band.width &&
     y>=band.y && y<band.y+band.height)return true;
 }
 return false;
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
