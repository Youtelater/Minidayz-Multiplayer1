#include "minidayz/engine.hpp"
#include "minidayz/world.hpp"
#include "minidayz/source_mapgen_geometry.hpp"
#include "minidayz/generated/terrain_scatter_source.hpp"
#include "minidayz/source_world_layer_depth.hpp"

#include <cstddef>
#include <cstdint>

namespace minidayz {

// Root Tilemap230 SetTile choices are painted during world generation.
// Moving the camera must not consume random numbers or re-roll terrain.
bool Renderer::draw_source_procedural_environment(
    const WorldSystem& world,float camera_x,float camera_y,
    float view_width,float view_height) noexcept {
 if(!world.generation_complete()||world.map_marker_count()==0)return true;
 if(view_width<=0.0f||view_height<=0.0f)return false;
 constexpr float kSourceTile=30.0f;
 constexpr int kPatchCells=35;
 const float view_right=camera_x+view_width;
 const float view_bottom=camera_y+view_height;
 for(const auto& patch:world.source_scatter_patches()){
  const auto* marker=world.map_marker(patch.marker_index);
  if(!marker)return false;
  const auto center=world_source::mapgen_hotspot(
      marker->cell_x,marker->cell_y);
  const float left=center.x+patch.offset_x;
  const float top=center.y+patch.offset_y;
  if(left>=view_right||top>=view_bottom||
     left+kPatchCells*kSourceTile<=camera_x||
     top+kPatchCells*kSourceTile<=camera_y)continue;
  for(const auto& tile:patch.tiles){
   if(tile.local_index>=kPatchCells*kPatchCells)return false;
   const int x=tile.local_index%kPatchCells;
   const int y=tile.local_index/kPatchCells;
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
   const auto frame=tile.original_frame;
   if(frame>=C2D_SpriteSheetCount(detail_sheets_[3]))return false;
   C2D_Sprite sprite{};
   C2D_SpriteFromSheet(&sprite,detail_sheets_[3],frame);
   if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f)return false;
   C2D_SpriteSetPos(&sprite,xx,yy);
   C2D_SpriteSetScale(&sprite,kSourceTile/sprite.params.pos.w,
                              kSourceTile/sprite.params.pos.h);
   C2D_SpriteSetDepth(&sprite,world_source::source_world_layer_depth(
       stage1::kSourceScatterGroundLayer,0,0));
   if(!C2D_DrawSprite(&sprite))return false;
  }
 }
 return true;
}

} // namespace minidayz
