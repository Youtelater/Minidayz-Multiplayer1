#include "minidayz/engine.hpp"
#include "minidayz/world.hpp"
#include "minidayz/generated/terrain_scatter_source.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <random>

namespace minidayz {

// The source create_map_location function creates ground_enviroment (230)
// at HotspotX-500, HotspotY-500, then issues Repeat/SetTile actions using
// floor(random(...)) for 30-pixel source cells. This renderer preserves the
// source marker branches, weighted choose() tiles, repeat counts and positions.
// Rare perk-dependent branches remain excluded by the source extractor.
bool Renderer::draw_source_procedural_environment(
    const WorldSystem& world,float camera_x,float camera_y,
    float view_width,float view_height) noexcept {
 if(!world.generation_complete()||world.map_marker_count()==0)return true;
 if(view_width<=0.0f||view_height<=0.0f)return false;
 const auto& grid=world.grid();
 const auto& bounds=world.bounds();
 if(!grid.width()||!grid.height()||bounds.width<=0.0f||bounds.height<=0.0f)
  return false;
 constexpr float kSourceTile=30.0f;
 constexpr float kHalfPatch=500.0f;
 constexpr int kPatchCells=33;
 constexpr std::uint16_t kEmpty=0xffffu;
 const float cell_w=bounds.width/grid.width();
 const float cell_h=bounds.height/grid.height();
 const auto view_right=camera_x+view_width;
 const auto view_bottom=camera_y+view_height;
 for(std::size_t index=0;index<world.map_marker_count();++index){
  const WorldMapMarker* marker=world.map_marker(index);
  if(!marker)continue;
  const float left=(marker->cell_x+0.5f)*cell_w-kHalfPatch;
  const float top=(marker->cell_y+0.5f)*cell_h-kHalfPatch;
  if(left>=view_right||top>=view_bottom||
     left+kPatchCells*kSourceTile<=camera_x||
     top+kPatchCells*kSourceTile<=camera_y)continue;
  ScatterCacheEntry* patch=nullptr;
  for(auto& candidate:scatter_cache_)
   if(candidate.valid && candidate.seed==world.seed() &&
      candidate.marker_index==index &&
      candidate.marker_type==marker->marker_type &&
      candidate.cell_x==marker->cell_x &&
      candidate.cell_y==marker->cell_y){patch=&candidate;break;}
  if(!patch){
   patch=&scatter_cache_[next_scatter_cache_++%kScatterCacheEntries];
   // Build the source-derived SetTile patch once on cache miss, then
   // reuse the exact result on every frame and while the camera pans.
   patch->valid=false;
   patch->cells.fill(kEmpty);
   patch->has_source_rule=false;
   std::uint32_t state=world.seed()^0x9e3779b9u;
   state^=static_cast<std::uint32_t>(marker->cell_x+1)*0x85ebca6bu;
   state^=static_cast<std::uint32_t>(marker->cell_y+1)*0xc2b2ae35u;
   state^=static_cast<std::uint32_t>(marker->marker_type)*0x27d4eb2du;
   std::minstd_rand rng(state?state:1u);
   for(const auto& rule:stage1::kTerrainScatterRules){
    if(rule.marker!=marker->marker_type)continue;
    patch->has_source_rule=true;
    if(rule.x_min>=rule.x_max||rule.y_min>=rule.y_max||
       rule.x_max>kPatchCells||rule.y_max>kPatchCells||
       rule.tile_count==0||
       rule.tile_offset+rule.tile_count>stage1::kTerrainScatterChoiceCount)
     return false;
    std::uniform_int_distribution<int> xdist(rule.x_min,rule.x_max-1);
    std::uniform_int_distribution<int> ydist(rule.y_min,rule.y_max-1);
    std::uniform_int_distribution<unsigned> choose(0,rule.tile_count-1);
    for(std::uint16_t n=0;n<rule.repeat;++n){
     const int x=xdist(rng);
     const int y=ydist(rng);
     const auto frame=stage1::kTerrainScatterChoices[
         rule.tile_offset+choose(rng)];
     patch->cells[static_cast<std::size_t>(y)*kPatchCells+x]=frame;
    }
   }
   patch->seed=world.seed();
   patch->marker_index=index;
   patch->marker_type=marker->marker_type;
   patch->cell_x=marker->cell_x;
   patch->cell_y=marker->cell_y;
   patch->valid=true;
  }
  if(!patch->has_source_rule)continue;
  // The generated 30px tile art shares the exact object-230 original
  // sheet with authored ground environment tiles, not a replacement PNG.
  for(int y=0;y<kPatchCells;++y)for(int x=0;x<kPatchCells;++x){
   const auto frame=patch->cells[static_cast<std::size_t>(y)*kPatchCells+x];
   if(frame==kEmpty)continue;
   const float xx=left+x*kSourceTile, yy=top+y*kSourceTile;
   if(xx>=view_right||yy>=view_bottom||
      xx+kSourceTile<=camera_x||yy+kSourceTile<=camera_y)continue;
   if(!detail_sheets_[3]){
    auto sheet=C2D_SpriteSheetLoad(
        "romfs:/gfx/original_map_detail_230.t3x");
    if(!sheet)return false;
    if(C2D_SpriteSheetCount(sheet)==0){
     C2D_SpriteSheetFree(sheet);return false;
    }
    detail_sheets_[3]=sheet;
   }
   if(frame>=C2D_SpriteSheetCount(detail_sheets_[3]))return false;
   C2D_Sprite sprite{};
   C2D_SpriteFromSheet(&sprite,detail_sheets_[3],frame);
   if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f)return false;
   C2D_SpriteSetPos(&sprite,xx,yy);
   C2D_SpriteSetScale(&sprite,kSourceTile/sprite.params.pos.w,
                              kSourceTile/sprite.params.pos.h);
   C2D_SpriteSetDepth(&sprite,0.825f);
   if(!C2D_DrawSprite(&sprite))return false;
  }
 }
 return true;
}

} // namespace minidayz
