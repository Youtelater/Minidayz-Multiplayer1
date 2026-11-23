#include "minidayz/engine.hpp"
#include "minidayz/generated/sprite_catalog.hpp"
#include "minidayz/generated/stage1_data.hpp"
#include "minidayz/generated/ground_floor_source.hpp"
#include "minidayz/terrain_tiling.hpp"
#include "minidayz/terrain_level.hpp"
#include "minidayz/terrain_roads.hpp"
#include "minidayz/source_mapgen_geometry.hpp"
#include "minidayz/terrain_source_tile_flags.hpp"
#include "minidayz/menu_bitmap_font.hpp"
#include "minidayz/terrain_source_visibility.hpp"
#include "minidayz/source_world_layer_depth.hpp"
#include "minidayz/generated/water_shore_source.hpp"
#include "minidayz/world.hpp"
#include "minidayz/generated/item_sprite_catalog.hpp"
#include "minidayz/generated/original_image_catalog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <utility>
#include <tuple>

namespace minidayz {
namespace {
// Draw a source Tilemap cell using its exact Construct 2 flip flags.
// Citro2D UV flips are controlled through negative sprite scales, and the
// diagonal transform is a centered 90-degree rotation plus a UV flip.
// Centering before scaling preserves tile bounds for every combination.
bool draw_original_tile(C2D_SpriteSheet sheet,std::uint32_t descriptor,
                        float x,float y,float size,float depth) noexcept {
 if(!sheet||descriptor==terrain::kTileEmpty||size<=0.0f)return false;
 const auto frame=terrain::source_tile_frame(descriptor);
 if(frame>=C2D_SpriteSheetCount(sheet))return false;
 C2D_Sprite sprite{};
 C2D_SpriteFromSheet(&sprite,sheet,frame);
 if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f)return false;
 const auto transform=terrain::source_tile_transform(descriptor);
 C2D_SpriteSetCenter(&sprite,0.5f,0.5f);
 C2D_SpriteSetScale(&sprite,
   transform.scale_x*size/sprite.params.pos.w,
   transform.scale_y*size/sprite.params.pos.h);
 C2D_SpriteSetPos(&sprite,x+size*0.5f,y+size*0.5f);
 C2D_SpriteSetRotation(&sprite,transform.angle_radians);
 C2D_SpriteSetDepth(&sprite,depth);
 return C2D_DrawSprite(&sprite);
}
} // namespace

bool Renderer::initialize() noexcept {
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) return false;
 if(!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)){ C3D_Fini(); return false; }
 C2D_Prepare();
 top_=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);
 bottom_=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
 if(!top_||!bottom_){ shutdown(); return false; }
 // Original source SetTile patches are generated with the world.
 debug_font_=C2D_FontLoadSystem(CFG_REGION_USA);
 if(debug_font_) debug_buffer_=C2D_TextBufNew(4096);
 if(debug_font_&&debug_buffer_) debug_ready_=true;
 initialized_=true;
 return true;
}
void Renderer::shutdown() noexcept {
 unload_all();
 for(auto& sheet:floor_sheets_){if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}}
 for(auto& sheet:base_floor_sheets_){if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}}
 for(auto& sheet:road_sheets_){if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}}
 for(auto& sheet:detail_sheets_){if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}}
 detail_maps_.clear(); detail_load_attempted_=false; detail_loaded_=false;
 ground_maps_.clear();ground_load_attempted_=false;ground_loaded_=false;
 ground_active_level_=255;
 for(auto& sheet:menu_sheets_){if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}}
 if(debug_buffer_) C2D_TextBufDelete(debug_buffer_);
 if(debug_font_) C2D_FontFree(debug_font_);
 debug_buffer_=nullptr; debug_font_=nullptr; debug_ready_=false;
 if(initialized_||top_||bottom_){ C2D_Fini(); C3D_Fini(); }
 top_=bottom_=nullptr; initialized_=false;
}
void Renderer::begin_frame() noexcept {
 sprite_cache_misses_=0;
 sprite_cache_failed_draws_=0;
 world_sprite_candidates_=world_sprite_requested_=0;
 world_sprite_rendered_=world_sprite_invalid_=0;
 sprite_cache_drawn_this_frame_.fill(false);
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
 C2D_TargetClear(top_,C2D_Color32(12,16,18,255));
 C2D_TargetClear(bottom_,C2D_Color32(18,22,24,255));
}
void Renderer::begin_screen(gfxScreen_t screen) noexcept {
 C2D_SceneBegin(screen==GFX_TOP?top_:bottom_);
 C2D_ViewReset();
 // Only the 400x240 TOP screen scrolls with the world. Previously the
 // 320x240 touch menu/inventory inherited the survivor's world camera,
 // moving every button and label thousands of pixels off-screen.
 if(screen==GFX_TOP)
     C2D_ViewTranslate(-camera_x_,-camera_y_);
}
void Renderer::end_frame() noexcept { C3D_FrameEnd(0); }
void Renderer::load_debug_overlay(std::uint64_t frame,float delta,const InputState& input,bool scene_active) noexcept {
 if(!debug_ready_) return;
 C2D_TextBufClear(debug_buffer_);
 char lines[512];
 std::snprintf(lines,sizeof(lines),
  "MINI DAYZ+  |  ENGINE DEBUG\n"
  "--------------------------------\n"
  "Status: RUNNING\n"
  "Frame: %llu\n"
  "Delta: %.2f ms\n"
  "Scene: %s\n"
  "Keys held: 0x%08lX\n"
  "Keys pressed: 0x%08lX\n"
  "Circle: X %d  Y %d\n"
  "Touch: %s  X %u  Y %u\n"
  "--------------------------------\n"
  "Bottom screen debug overlay",
  static_cast<unsigned long long>(frame),delta*1000.0f,
  scene_active?"attached":"none",
  static_cast<unsigned long>(input.held),
  static_cast<unsigned long>(input.pressed),
  input.circle.dx,input.circle.dy,input.touching?"yes":"no",
  static_cast<unsigned>(input.touch.px),static_cast<unsigned>(input.touch.py));
 C2D_TextParse(&debug_text_,debug_buffer_,lines);
 C2D_TextOptimize(&debug_text_);
}
void Renderer::draw_debug_overlay() noexcept {
 // Always draw a visible diagnostics panel, even if the system font cannot
 // be loaded by an emulator. This makes font failure distinguishable from
 // a screen/target failure.
 draw_solid_rect(0.0f,0.0f,320.0f,240.0f,C2D_Color32(12,18,22,255));
 draw_solid_rect(6.0f,6.0f,308.0f,28.0f,C2D_Color32(35,75,58,255));
 draw_solid_rect(10.0f,42.0f,300.0f,2.0f,C2D_Color32(80,120,100,255));
 draw_solid_rect(10.0f,54.0f,220.0f,8.0f,C2D_Color32(45,90,70,255));
 draw_solid_rect(10.0f,72.0f,180.0f,8.0f,C2D_Color32(45,90,70,255));
 draw_solid_rect(10.0f,90.0f,260.0f,8.0f,C2D_Color32(45,90,70,255));
 draw_solid_rect(10.0f,108.0f,140.0f,8.0f,C2D_Color32(45,90,70,255));
 draw_solid_rect(10.0f,126.0f,290.0f,2.0f,C2D_Color32(80,120,100,255));
 if(debug_ready_) C2D_DrawText(&debug_text_,C2D_AlignLeft,12.0f,12.0f,0.90f,0.38f,0.38f);
}
void Renderer::draw_solid_rect(float x,float y,float width,float height,u32 color,float depth) noexcept {
 if(width>0.0f&&height>0.0f) C2D_DrawRectSolid(x,y,depth,width,height,color);
}
void Renderer::draw_text(const char* value,float x,float y,float scale) noexcept {
 if(!value||!value[0]||!(scale>0.0f))return;
 if(debug_ready_) {
  C2D_TextBufClear(debug_buffer_);
  C2D_TextParse(&debug_text_,debug_buffer_,value);
  C2D_TextOptimize(&debug_text_);
  C2D_DrawText(&debug_text_,C2D_AlignLeft,x,y,0.90f,scale,scale);
  return;
 }
 // Some iOS/3DS emulators cannot provide the region system font at all.
 // A menu made of textured buttons without text is functionally unusable.
 // This 5x7 fallback draws labels directly, without any external font file.
 // A 5x7 pixel-font must draw at integer screen pixels: fractional
 // 0.7-0.9px quads lose columns on GPU rasterization (e.g. CONTINUE).
 const float pixel=std::max(1.0f,std::round(scale*2.1f));
 float at_x=std::round(x),at_y=std::round(y);
 const u32 ink=C2D_Color32(243,234,211,255);
 for(const char* p=value;*p;++p) {
  if(*p=='\\n'){at_x=x;at_y+=pixel*9.0f;continue;}
  if(at_x>410.0f)break;
  char upper=*p;
  if(upper>='a'&&upper<='z')upper=static_cast<char>(upper-'a'+'A');
  const auto* glyph=menu::ui_font_glyph(upper);
  if(glyph)for(int row=0;row<7;++row)
   for(int col=0;col<5;++col)
    if(glyph[row]&(1u<<(4-col)))
     C2D_DrawRectSolid(at_x+col*pixel,at_y+row*pixel,0.90f,
                       pixel,pixel,ink);
  at_x+=pixel*6.0f;
 }
}
bool Renderer::load_sheet(std::size_t slot,const char* path) noexcept {
 if(!initialized_||slot>=kMaxSheets||!path||!path[0]) return false;
 C2D_SpriteSheet fresh=C2D_SpriteSheetLoad(path); if(!fresh) return false;
 if(sheets_[slot]) C2D_SpriteSheetFree(sheets_[slot]); sheets_[slot]=fresh; return true;
}
void Renderer::unload_sheet(std::size_t slot) noexcept {
 if(slot>=kMaxSheets) return;
 if(sheets_[slot]) C2D_SpriteSheetFree(sheets_[slot]);
 sheets_[slot]=nullptr;
 // Keep the name cache synchronized with the actual loaded sheet. Without
 // this, an evicted sprite atlas could be mistaken for a still-loaded one.
 sprite_cache_[slot]={};
}
void Renderer::unload_all() noexcept { for(std::size_t i=0;i<kMaxSheets;++i) unload_sheet(i); }
// Never destroy a T3X referenced by this frame's queued GPU draw calls.
// This bounds GPU memory while preventing silent house/roof texture reuse.
std::size_t Renderer::choose_sprite_cache_slot() noexcept {
 std::size_t candidate=kMaxSheets;
 for(std::size_t offset=0;offset<kMaxSheets;++offset){
  const auto i=(next_sprite_cache_+offset)%kMaxSheets;
  if(sprite_cache_drawn_this_frame_[i])continue;
  if(!sheets_[i]){candidate=i;break;}
  if(candidate==kMaxSheets)candidate=i;
 }
 if(candidate==kMaxSheets)return candidate;
 next_sprite_cache_=(candidate+1)%kMaxSheets;
 unload_sheet(candidate);
 return candidate;
}
// Share one atlas load path between real sprite draws and critical source
// overlays that must remain resident until their late Map-layer draw.
std::size_t Renderer::acquire_source_sprite_page(const char* sheet) noexcept {
 if(!sheet||!sheet[0])return kMaxSheets;
 for(std::size_t i=0;i<kMaxSheets;++i){
  if(sprite_cache_[i].sheet[0]&&
     std::strcmp(sprite_cache_[i].sheet,sheet)==0 && sheets_[i])
   return i;
 }
 ++sprite_cache_misses_;
 const auto slot=choose_sprite_cache_slot();
 if(slot==kMaxSheets){++sprite_cache_failed_draws_;return kMaxSheets;}
 char path[192]{};
 const int n=std::snprintf(path,sizeof(path),"romfs:/gfx/%s.t3x",sheet);
 if(n<=0||static_cast<std::size_t>(n)>=sizeof(path)||
    !load_sheet(slot,path)){++sprite_cache_failed_draws_;return kMaxSheets;}
 std::snprintf(sprite_cache_[slot].sheet,sizeof(sprite_cache_[slot].sheet),"%s",sheet);
 sprite_cache_[slot].slot=slot;
 return slot;
}

bool Renderer::draw_sprite(std::size_t slot,std::size_t image,float x,float y,float sx,float sy,float rotation,float depth,float opacity,float rgb_multiplier) noexcept {
 if(slot>=kMaxSheets||!sheets_[slot]||image>=C2D_SpriteSheetCount(sheets_[slot])) return false;
 if(!std::isfinite(opacity)||opacity<=0.0f)return true;
 sprite_cache_drawn_this_frame_[slot]=true;
 C2D_Sprite sprite{}; C2D_SpriteFromSheet(&sprite,sheets_[slot],image); C2D_SpriteSetPos(&sprite,x,y);
 C2D_SpriteSetScale(&sprite,sx,sy); C2D_SpriteSetRotation(&sprite,rotation); C2D_SpriteSetDepth(&sprite,depth);
 if(rgb_multiplier<1.0f){
  // Root Tint shader multiplies the original RGB and preserves alpha.
  C2D_ImageTint tint{};
  const float rgb=std::clamp(rgb_multiplier,0.0f,1.0f);
  C2D_PlainImageTint(&tint,C2D_Color32f(rgb,rgb,rgb,std::clamp(opacity,0.0f,1.0f)),1.0f);
  C2D_SetTintMode(C2D_TintMult);
  const bool drawn=C2D_DrawSpriteTinted(&sprite,&tint);
  C2D_SetTintMode(C2D_TintSolid);
  return drawn;
 }
 if(opacity>=1.0f)return C2D_DrawSprite(&sprite);
 C2D_ImageTint tint{};
 C2D_AlphaImageTint(&tint,std::clamp(opacity,0.0f,1.0f));
 return C2D_DrawSpriteTinted(&sprite,&tint);
}
bool Renderer::draw_source_sprite(std::uint16_t object,std::uint16_t animation,std::uint32_t frame,float x,float y,float sx,float sy,float depth,float rotation,float opacity,float rgb_multiplier,bool outline) noexcept {
 // A source Sprite with zero opacity is a logic-only mask or hidden
 // editor template. Never allocate GPU texture pages for invisible pixels.
 if(std::isfinite(opacity) && opacity<=0.0f)return true;
 const auto key=std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{object,animation,frame};
 const auto* begin=stage1::kSpriteFrames;
 const auto* end=begin+stage1::kSpriteFrameCount;
 const auto* found=std::lower_bound(begin,end,key,[](const stage1::SpriteFrameDef& item,const auto& wanted){
  return std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{item.object,item.animation,item.frame}<wanted;
 });
 if(found==end||found->object!=object||found->animation!=animation||found->frame!=frame){++sprite_cache_failed_draws_;return false;}
 const auto* selected=found;
 char effect_sheet[128]{};
 const char* sheet=selected->sheet;
 if(outline){
  const int n=std::snprintf(effect_sheet,sizeof(effect_sheet),"%s_outline",sheet);
  if(n<=0 || static_cast<std::size_t>(n)>=sizeof(effect_sheet))return false;
  sheet=effect_sheet;
 }
 auto slot=acquire_source_sprite_page(sheet);
 // Preserve the selected survivor and animation when its outline page is unavailable.
 if(slot==kMaxSheets && outline)
  slot=acquire_source_sprite_page(selected->sheet);
 if(slot==kMaxSheets)return false;
 // Construct Sprite rotates about original active-frame hotspot (X,Y).
 // Uncentered Citro2D rotates about draw origin; keep pivot and culling
 // consistent using the same hotspot-corrected transform.
 const auto origin=terrain::source_sprite_draw_origin(
     x,y,selected->width,selected->height,
     selected->origin_x,selected->origin_y,sx,sy,rotation);
 return draw_sprite(slot,selected->image,origin.x,origin.y,
                     sx*selected->texture_scale_x,sy*selected->texture_scale_y,rotation,depth,opacity,rgb_multiplier);
}
// Reserve an original source atlas for a later foreground overlay without
// submitting fake off-screen geometry. A scene with >32 distinct textures
// otherwise starves its night/rain layer after dense trees/buildings draw.
bool Renderer::pin_source_sprite_texture(std::uint16_t object,
                                         std::uint16_t animation,
                                         std::uint32_t frame) noexcept {
 const auto key=std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{
     object,animation,frame};
 const auto* begin=stage1::kSpriteFrames;
 const auto* end=begin+stage1::kSpriteFrameCount;
 const auto* found=std::lower_bound(begin,end,key,
     [](const stage1::SpriteFrameDef& item,const auto& wanted){
       return std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{
           item.object,item.animation,item.frame}<wanted;
     });
 if(found==end||found->object!=object||found->animation!=animation||
    found->frame!=frame){
  ++sprite_cache_failed_draws_;
  return false;
 }
 const auto slot=acquire_source_sprite_page(found->sheet);
 if(slot==kMaxSheets)return false;
 // A pinned sheet must not be recycled before the late Rain44/Night46
 // draw; begin_frame resets these pins when queued GPU commands finish.
 sprite_cache_drawn_this_frame_[slot]=true;
 return true;
}
bool Renderer::draw_original_image(const char* original_png,float x,float y,
                                   float width,float height,float depth) noexcept {
 // Every canonical assets/gfx PNG is indexed once by its original filename.
 // This handles artwork not represented by a Construct animation record.
 if(!original_png||!original_png[0]||!(width>0.0f&&height>0.0f))
  return false;
 const auto* begin=stage1::kOriginalImages;
 const auto* end=begin+stage1::kOriginalImageCount;
 const auto* selected=std::lower_bound(begin,end,original_png,
    [](const stage1::OriginalImageAsset& asset,const char* name){
     return std::strcmp(asset.png,name)<0;
    });
 if(selected==end||std::strcmp(selected->png,original_png)!=0)return false;
 // Never draw an entire Construct sprite sheet or tilemap as a single sprite.
 // Use the source animation/frame reference instead. This rejects the exact
 // kind of image mixup that previously produced striped UI and false items.
 if(selected->is_source_atlas)return false;
 std::size_t slot=kMaxSheets;
 for(std::size_t i=0;i<kMaxSheets;++i){
  if(sheets_[i]&&sprite_cache_[i].sheet[0]&&
     std::strcmp(sprite_cache_[i].sheet,selected->texture)==0){
   slot=i;break;
  }
 }
 if(slot==kMaxSheets){
  slot=choose_sprite_cache_slot();
  if(slot==kMaxSheets){++sprite_cache_failed_draws_;return false;}
  char path[192]{};
  const auto length=std::snprintf(path,sizeof(path),"romfs:/gfx/%s",
                                   selected->texture);
  if(length<=0||static_cast<std::size_t>(length)>=sizeof(path)||
     !load_sheet(slot,path)){++sprite_cache_failed_draws_;return false;}
  std::snprintf(sprite_cache_[slot].sheet,
                sizeof(sprite_cache_[slot].sheet),"%s",selected->texture);
  sprite_cache_[slot].slot=slot;
 }
 if(C2D_SpriteSheetCount(sheets_[slot])==0)return false;
 C2D_Sprite sprite{};
 C2D_SpriteFromSheet(&sprite,sheets_[slot],0);
 if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f)return false;
 C2D_SpriteSetPos(&sprite,x,y);
 C2D_SpriteSetScale(&sprite,width/sprite.params.pos.w,
                            height/sprite.params.pos.h);
 C2D_SpriteSetDepth(&sprite,depth);
 sprite_cache_drawn_this_frame_[slot]=true;
 return C2D_DrawSprite(&sprite);
}

void Renderer::release_menu_sheets() noexcept {
 for(auto& sheet:menu_sheets_) {
  if(!sheet)continue;
  C2D_SpriteSheetFree(sheet);
  sheet=nullptr;
 }
}
bool Renderer::draw_menu_art(std::size_t index,float x,float y,float width,float height,float depth) noexcept {
 // Original assets/gfx menu sprites, already converted into romfs/gfx.
 static constexpr const char* paths[]={
  "romfs:/gfx/main_menu_logo_sheet0.t3x",
  "romfs:/gfx/menu_btn_wood_single.t3x",
  "romfs:/gfx/tiledbgmenu_city_up.t3x"
 };
 if(index>=3||width<=0.0f||height<=0.0f) return false;
 // Real Manic Emu footage revealed the reversed depth assumption:
 // larger Citro2D Z appears IN FRONT. Put the original wooden button
 // above its 0.5f colored hitbox; text is at 0.90f.
 if(index==1 && depth<=0.5f)depth=0.75f;
 if(!menu_sheets_[index]) menu_sheets_[index]=C2D_SpriteSheetLoad(paths[index]);
 if(!menu_sheets_[index]||C2D_SpriteSheetCount(menu_sheets_[index])==0) return false;
 C2D_Sprite sprite{};
 C2D_SpriteFromSheet(&sprite,menu_sheets_[index],0);
 const float sw=sprite.params.pos.w, sh=sprite.params.pos.h;
 if(sw<=0.0f||sh<=0.0f) return false;
 C2D_SpriteSetPos(&sprite,x,y);
 C2D_SpriteSetScale(&sprite,width/sw,height/sh);
 C2D_SpriteSetDepth(&sprite,depth);
 return C2D_DrawSprite(&sprite);
}
bool Renderer::draw_original_ground_floor(float camera_x,float camera_y,float width,float height,std::uint8_t source_level,float second_half_x) noexcept {
 // Exact original Construct Map ground tile placements, generated from the
 // root data.deobfuscated.js into a compact 3DS RomFS tilemap.
 if(width<=0.0f||height<=0.0f) return false;
 if(!ground_load_attempted_){
  ground_load_attempted_=true;
  std::FILE* file=std::fopen("romfs:/gfx/original_map_ground_layout.bin","rb");
  if(!file) return false;
  auto read_u32=[&](std::uint32_t& value) -> bool {
   return std::fread(&value,sizeof(value),1,file)==1;
  };
  char magic[4]{};
  std::uint32_t size=0,pages=0,count=0;
  bool valid=std::fread(magic,1,4,file)==4 &&
             std::memcmp(magic,"MDG3",4)==0 &&
             read_u32(size)&&read_u32(pages)&&read_u32(count) &&
             size>0&&size<=256&&pages>0&&pages<=8&&count>0&&count<=32;
  if(valid){
   ground_tile_pixels_=size;ground_pages_=pages;
   for(std::uint32_t i=0;i<count&&valid;++i){
    GroundMap map{};
    valid=std::fread(&map.x,sizeof(float),1,file)==1 &&
          std::fread(&map.y,sizeof(float),1,file)==1 &&
          read_u32(map.columns)&&read_u32(map.rows)&&read_u32(map.object_id);
    if(!valid||!std::isfinite(map.x)||!std::isfinite(map.y)||
       (map.object_id!=482 && map.object_id!=687 &&
        map.object_id!=688 && map.object_id!=776 &&
        map.object_id!=980 && map.object_id!=1138 &&
        map.object_id!=1139)||
       map.columns==0||map.rows==0||
       map.columns>4096||map.rows>4096||
       static_cast<std::uint64_t>(map.columns)*map.rows>2000000u){
     valid=false;break;
    }
    map.cells.resize(static_cast<std::size_t>(map.columns)*map.rows);
    valid=std::fread(map.cells.data(),sizeof(std::uint32_t),
                     map.cells.size(),file)==map.cells.size();
    if(valid) ground_maps_.push_back(std::move(map));
   }
  }
  std::fclose(file);
  // Parsed source tilemap references are retained here, but the texture
  // atlas pages are loaded only if a source-authored tile actually enters
  // the 400x240 viewport. Eagerly loading all atlas pages can leave no
  // texture memory for the player, items, or procedurally placed objects.
  if(!valid){
   for(auto& sheet:base_floor_sheets_){
    if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}
   }
   for(auto& sheet:floor_sheets_){
    if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}
   }
   ground_maps_.clear();
   return false;
  }
  ground_loaded_=true;
 }
 if(!ground_loaded_) return false;
 if(ground_active_level_!=source_level){
  // Authored ground atlases are shared source data, but no old-world page
  // should remain resident after a transition on memory-limited hardware.
  for(auto& sheet:floor_sheets_){
   if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}
  }
  ground_active_level_=source_level;
 }
 // Traveling between source maps must not accumulate seven full ground
 // sheets plus seven road sheets in the 3DS GPU texture pool. Keep only
 // the active world theme (and the final world's second-half theme).
 const auto primary_theme=terrain::active_ground_texture(source_level);
 for(std::uint32_t i=0;i<7;++i){
  if(i==primary_theme || (source_level==5 && i==5)) continue;
  if(base_floor_sheets_[i]){
   C2D_SpriteSheetFree(base_floor_sheets_[i]);
   base_floor_sheets_[i]=nullptr;
  }
  if(road_sheets_[i]){
   C2D_SpriteSheetFree(road_sheets_[i]);
   road_sheets_[i]=nullptr;
  }
 }
 // Keep only the needed original ground theme resident. Loading all seven
 // terrain sheets at startup can exhaust original 3DS texture memory and
 // make every map fail to render, even when the current sheet is valid.
 // The final source map uses a second theme east of its column boundary.
 const auto ensure_ground_sheet=[&](std::uint32_t theme) noexcept -> bool {
  if(theme>=7) return false;
  if(base_floor_sheets_[theme]) return true;
  char path[96]{};
  if(theme==0)
   std::snprintf(path,sizeof(path),"romfs:/gfx/original_map_base_ground.t3x");
  else
   std::snprintf(path,sizeof(path),
     "romfs:/gfx/original_map_base_ground_lvl%u.t3x",
     static_cast<unsigned>(theme+1));
  auto sheet=C2D_SpriteSheetLoad(path);
  if(!sheet) return false;
  if(C2D_SpriteSheetCount(sheet)==0){C2D_SpriteSheetFree(sheet);return false;}
  base_floor_sheets_[theme]=sheet;
  return true;
 };
 if(!ensure_ground_sheet(terrain::active_ground_texture(source_level))) return false;
 const float tile=static_cast<float>(ground_tile_pixels_);
 const float base_tile=stage1::kOriginalFloorBaseTileSize;
 // Source Map object 482 has only two authored ground strips. The original
 // game's Construct events dynamically populate the rest of each new map.
 // Tile the ORIGINAL grass image beneath those authored strips rather than
 // showing a black GPU-clear color at virtually every new spawn.
 TerrainTileWindow visible{};
 if(!calculate_terrain_tile_window(camera_x,camera_y,width,height,base_tile,
                                   visible)) return false;
 for(int row=visible.first_row;row<visible.last_row;++row)
  for(int col=visible.first_column;col<visible.last_column;++col){
   const float tile_world_x=(static_cast<float>(col)+0.5f)*base_tile;
   const auto theme=terrain::ground_texture_at(
      source_level,tile_world_x,second_half_x);
   if(!ensure_ground_sheet(theme)) return false;
   C2D_Sprite sprite{};
   C2D_SpriteFromSheet(&sprite,base_floor_sheets_[theme],0);
   if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f) return false;
   C2D_SpriteSetPos(&sprite,col*base_tile,row*base_tile);
   C2D_SpriteSetScale(&sprite,base_tile/sprite.params.pos.w,
                            base_tile/sprite.params.pos.h);
   C2D_SpriteSetDepth(&sprite,0.04f);
   if(!C2D_DrawSprite(&sprite)) return false;
  }
 // Preserve the exact source RLE cells at their original Map positions.
 for(const GroundMap& map:ground_maps_){
  // Source tilemap objects can cross the split-world seam. Resolve the
  // original terrain object per visible column, not once at map origin.
  const int x0=std::max(0,static_cast<int>(std::floor((camera_x-map.x)/tile)));
  const int y0=std::max(0,static_cast<int>(std::floor((camera_y-map.y)/tile)));
  const int x1=std::min(static_cast<int>(map.columns),
              static_cast<int>(std::ceil((camera_x+width-map.x)/tile))+1);
  const int y1=std::min(static_cast<int>(map.rows),
              static_cast<int>(std::ceil((camera_y+height-map.y)/tile))+1);
  for(int row=y0;row<y1;++row) for(int col=x0;col<x1;++col){
   const auto map_theme=terrain::ground_texture_at(
       source_level,map.x+(static_cast<float>(col)+0.5f)*tile,
       second_half_x);
   if(map.object_id!=terrain::kGroundObjectByTexture[map_theme]) continue;
   const std::uint32_t descriptor=map.cells[
     static_cast<std::size_t>(row)*map.columns+static_cast<std::size_t>(col)];
   if(descriptor==0xffffffffu) continue;
   const std::uint32_t page=terrain::source_tile_page(descriptor);
   if(page>=ground_pages_||page>=8u) continue;
   if(!floor_sheets_[page]){
    char path[80]{};
    std::snprintf(path,sizeof(path),
                  "romfs:/gfx/original_map_ground_p%03u.t3x",page);
    floor_sheets_[page]=C2D_SpriteSheetLoad(path);
    if(!floor_sheets_[page]) return false;
   }
   if(!draw_original_tile(floor_sheets_[page],descriptor,
                           map.x+col*tile,map.y+row*tile,
                           tile,0.08f))return false;
  }
 }
 return true;
}
bool Renderer::draw_original_environment_tilemaps(
    float camera_x,float camera_y,float width,float height) noexcept {
 if(!(width>0.0f&&height>0.0f))return false;
 if(!detail_load_attempted_){
  detail_load_attempted_=true;
  constexpr std::uint16_t kOriginalObjects[4]={208,211,418,230};
  std::FILE* file=std::fopen("romfs:/gfx/original_map_details_layout.bin","rb");
  if(!file)return false;
  char magic[4]{};
  std::uint32_t count=0;
  bool valid=std::fread(magic,1,4,file)==4 &&
             std::memcmp(magic,"MDT1",4)==0 &&
             std::fread(&count,sizeof(count),1,file)==1 &&
             count>0 && count<=32;
  for(std::uint32_t i=0;i<count&&valid;++i){
   DetailMap map{};
   valid=std::fread(&map.object_id,sizeof(map.object_id),1,file)==1 &&
         std::fread(&map.layer,sizeof(map.layer),1,file)==1 &&
         std::fread(&map.x,sizeof(map.x),1,file)==1 &&
         std::fread(&map.y,sizeof(map.y),1,file)==1 &&
         std::fread(&map.tile_size,sizeof(map.tile_size),1,file)==1 &&
         std::fread(&map.columns,sizeof(map.columns),1,file)==1 &&
         std::fread(&map.rows,sizeof(map.rows),1,file)==1;
   if(!valid||!std::isfinite(map.x)||!std::isfinite(map.y)||
      map.tile_size<8||map.tile_size>256||
      map.columns==0||map.rows==0||
      map.columns>4096||map.rows>4096||
      static_cast<std::uint64_t>(map.columns)*map.rows>2000000u){
    valid=false; break;
   }
   bool object_valid=false;
   for(const auto object:kOriginalObjects)
    if(map.object_id==object)object_valid=true;
   if(!object_valid){valid=false;break;}
   const std::size_t cell_count =
       static_cast<std::size_t>(map.columns)*map.rows;
   // The source Map's layer 44+ contains offscreen overlays, triggers and
   // hidden tilemap instances. The world renderer never draws these.
   // Skip their packed cells *before* allocating, rather than reserving
   // hundreds of thousands of invisible cells on original 3DS hardware.
   if(map.layer>=44){
    if(std::fseek(file,static_cast<long>(cell_count*sizeof(std::uint32_t)),
                  SEEK_CUR)!=0){valid=false;break;}
    continue;
   }
   map.cells.resize(cell_count);
   if(std::fread(map.cells.data(),sizeof(std::uint32_t),
                 map.cells.size(),file)!=map.cells.size()){
    valid=false;break;
   }
   detail_maps_.push_back(std::move(map));
  }
  std::fclose(file);
  // Environment atlas sheets are loaded only when nonempty source cells
  // enter the viewport. Four eager RGBA atlases can exhaust the original
  // 3DS GPU texture pool before trees, items, and ground render.
  if(!valid){
   for(auto& sheet:detail_sheets_){
    if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}
   }
   detail_maps_.clear();
   return false;
  }
  detail_loaded_=true;
 }
 if(!detail_loaded_)return false;
 constexpr std::uint16_t kOriginalObjects[4]={208,211,418,230};
 for(const DetailMap& map:detail_maps_){
  // Original browser UI/rain event layers must not become static world
  // scenery on the top 3DS screen.
  if(map.layer>=44)continue;
  std::size_t slot=4;
  for(std::size_t k=0;k<4;++k)
   if(kOriginalObjects[k]==map.object_id){slot=k;break;}
  if(slot>=4)continue;
  const float tile=static_cast<float>(map.tile_size);
  const int x0=std::max(0,static_cast<int>(
      std::floor((camera_x-map.x)/tile))-1);
  const int y0=std::max(0,static_cast<int>(
      std::floor((camera_y-map.y)/tile))-1);
  const int x1=std::min(static_cast<int>(map.columns),
      static_cast<int>(std::ceil((camera_x+width-map.x)/tile))+1);
  const int y1=std::min(static_cast<int>(map.rows),
      static_cast<int>(std::ceil((camera_y+height-map.y)/tile))+1);
  if(x1<=x0||y1<=y0)continue;
  // Tilemaps use the SAME Construct Map layer-to-Z conversion as sprites.
  // The former independent 0.12+0.008*layer scale placed original layer-17
  // tiles BEHIND layer-9 building shadows. Preserve the original source
  // ordering (shadows 9, foundation 17, water 19) without fake shadow art.
  // The base of a layer stays behind its same-layer placed sprites.
  const float depth=world_source::source_world_layer_depth(map.layer,0,0);
  for(int row=y0;row<y1;++row)for(int col=x0;col<x1;++col){
   auto frame=map.cells[static_cast<std::size_t>(row)*
                         map.columns+static_cast<std::size_t>(col)];
   if(map.object_id==208 && frame==0xffffffffu){
    // The original water Tilemap has SetTile shoreline rules based on
    // Compare tile at x +/- 1 and y +/- 1 against source water frame 4.
    // Only fill currently EMPTY source cells; never overwrite the exact
    // RLE-authored water, beach or shoreline frames.
    for(const auto& rule:stage1::kWaterShoreRules){
     // Some source branches explicitly handle zero surrounding water.
     // They belong to the larger procedural water-creation event and must
     // not be applied to every empty cell in an authored Map tilemap.
     bool has_source_water=false;
     for(std::uint16_t i=0;i<rule.count;++i)
      if(stage1::kWaterShoreChecks[rule.first+i].requires_water)
       has_source_water=true;
     if(!has_source_water)continue;
     bool match=true;
     for(std::uint16_t i=0;i<rule.count;++i){
      const auto& check=stage1::kWaterShoreChecks[rule.first+i];
      const int nx=col+check.dx,ny=row+check.dy;
      const bool water=nx>=0&&ny>=0&&
          nx<static_cast<int>(map.columns)&&
          ny<static_cast<int>(map.rows)&&
          map.cells[static_cast<std::size_t>(ny)*map.columns+
                    static_cast<std::size_t>(nx)]!=terrain::kTileEmpty &&
          terrain::source_tile_frame(map.cells[
              static_cast<std::size_t>(ny)*map.columns+
              static_cast<std::size_t>(nx)])==4u;
      if(water!=check.requires_water){match=false;break;}
     }
     if(match){frame=rule.frame;break;}
    }
   }
   if(frame==0xffffffffu)continue;
   if(!detail_sheets_[slot]){
    char path[80]{};
    std::snprintf(path,sizeof(path),
                  "romfs:/gfx/original_map_detail_%u.t3x",
                  static_cast<unsigned>(kOriginalObjects[slot]));
    auto sheet=C2D_SpriteSheetLoad(path);
    if(!sheet) return false;
    if(C2D_SpriteSheetCount(sheet)==0){C2D_SpriteSheetFree(sheet);return false;}
    detail_sheets_[slot]=sheet;
   }
   if(!draw_original_tile(detail_sheets_[slot],frame,
                           map.x+col*tile,map.y+row*tile,
                           tile,depth))return false;
  }
 }
 return true;
}
bool Renderer::draw_source_road_grid(const WorldGrid& grid,
                                    const WorldBounds& bounds,
                                    float camera_x,float camera_y,
                                    float view_width,float view_height,
                                    std::uint8_t source_level) noexcept {
 // Roads use independent original road atlases. A damaged/missing ground
 // layout must NOT suppress an otherwise valid generated road network.
 // The scene already draws a visible floor-load diagnostic independently.
 if(grid.width()==0||grid.height()==0) return false;
 if(!(bounds.width>0.0f && bounds.height>0.0f)) return false;
 // Cache load failures for this pass so a missing theme cannot suppress
 // the other half of the map or cause repeated ROMFS reads per tile.
 bool missing_theme[7]{};
 bool roads_drawn=true;
 const auto ensure_road_sheet=[&](std::uint32_t theme) noexcept -> bool {
  if(theme>=7 || missing_theme[theme]) return false;
  if(road_sheets_[theme]) return true;
  char name[80]{};
  std::snprintf(name,sizeof(name),"romfs:/gfx/original_map_road_lvl%u.t3x",
                static_cast<unsigned>(theme+1));
  auto sheet=C2D_SpriteSheetLoad(name);
  if(!sheet || C2D_SpriteSheetCount(sheet)==0){
   if(sheet)C2D_SpriteSheetFree(sheet);
   missing_theme[theme]=true;
   return false;
  }
  road_sheets_[theme]=sheet;
  return true;
 };
 // The original logical Array Y axis is physically east-west on Map.
 // On the final map, the second ground theme starts at logical row 18.
 const float second_half_x=(source_level==5 && grid.height()==33)
     ? world_source::mapgen_hotspot(0,18).x : 1.0e30f;
 // Original 1066.SetTileRange paints 17x2 / 2x17 rectangles;
 // never collapse this into one source tile of pavement.
 const float road_width=world_source::source_road_painted_width(
     stage1::kOriginalFloorTileSize);
 if(!(road_width>0.0f))return false;
 // Original Map asphalt is a separate Construct layer, before
 // ground_enviroment. Resolve the source layer rather than choosing
 // an arbitrary 3DS Z such as 0.10, which can break layer ordering.
 const stage1::LayoutDef* original_map=nullptr;
 for(const auto& layout:stage1::kLayoutTable)
  if(std::strcmp(layout.name,"Map")==0){original_map=&layout;break;}
 if(!original_map)return false;
 const auto original_layout=static_cast<std::uint16_t>(
      original_map-stage1::kLayoutTable);
 const stage1::LayerDef* asphalt_source_layer=nullptr;
 for(const auto& source_layer:stage1::kLayerTable)
  if(source_layer.layout==original_layout &&
     std::strcmp(source_layer.name,"asphalt")==0){
    asphalt_source_layer=&source_layer;break;
  }
 if(!asphalt_source_layer)return false;
 const float road_depth=world_source::source_world_layer_depth(
     asphalt_source_layer->index,0,0);
 // The source physical grid axes are transposed. Scan at most 16x33
 // logical cells, then reject every band outside the viewport.
 // Keep the original Map asphalt-layer depth, behind the authored
 // ground_enviroment layer but in front of the base ground floor.
 // Render only clipped, camera-visible source Array road corridor pieces.
 for(int row=0;row<grid.height();++row)
  for(int col=0;col<grid.width();++col){
  const std::uint16_t code=grid.at(col,row);
  if(!terrain::horizontal_road(code)&&!terrain::vertical_road(code))
   continue;
  // Source code12/13 has a full 17-cell base followed by four
  // optional 9-tile arms. Code14 has only four neighbor-gated arms.
  // Draw in original action order and omit overlapping GPU submissions.
  for(int segment=0;segment<5;++segment){
   const auto band=world_source::source_grid_road_segment(
       grid,col,row,segment,road_width);
   if(!terrain::intersects_view(band,camera_x,camera_y,
                                view_width,view_height))continue;
   // Repeat native 60px Construct stone tiles instead of stretching
   // one source tile across a 2300px world cell. Limit draw calls to
   // the pieces visible in the 400x240 native top-screen camera.
   const float tile=stage1::kOriginalFloorTileSize;
   const int first_x=std::max(0,static_cast<int>(
       std::floor((camera_x-band.x)/tile)));
   const int first_y=std::max(0,static_cast<int>(
       std::floor((camera_y-band.y)/tile)));
   const int last_x=std::min(static_cast<int>(std::ceil(band.width/tile)),
       static_cast<int>(std::ceil((camera_x+view_width-band.x)/tile)));
   const int last_y=std::min(static_cast<int>(std::ceil(band.height/tile)),
       static_cast<int>(std::ceil((camera_y+view_height-band.y)/tile)));
   for(int y=first_y;y<last_y;++y)
    for(int x=first_x;x<last_x;++x){
     const float world_tile_x=band.x+(static_cast<float>(x)+0.5f)*tile;
     const auto theme=terrain::ground_texture_at(
         source_level,world_tile_x,second_half_x);
     if(!ensure_road_sheet(theme)){
      roads_drawn=false;
      continue;
     }
     C2D_Sprite sprite{};
     C2D_SpriteFromSheet(&sprite,road_sheets_[theme],0);
     if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f){
      missing_theme[theme]=true;
      roads_drawn=false;
      continue;
     }
     // A source grid cell is not necessarily a multiple of its 60px
     // road tile. Trim the final tile instead of painting beyond the
     // original corridor, which made road edges bleed into water/forest.
     // Never submit the same 2x2 junction texel twice when multiple
     // source code14 neighbor guards select overlapping SetTileRange arms.
     if(world_source::source_grid_road_tile_was_drawn(
           grid,col,row,segment,band.x+x*tile,band.y+y*tile,
           tile,road_width))continue;
     const float clipped_w=std::min(tile,band.width-x*tile);
     const float clipped_h=std::min(tile,band.height-y*tile);
     if(clipped_w<=0.0f||clipped_h<=0.0f)continue;
     C2D_SpriteSetPos(&sprite,band.x+x*tile,band.y+y*tile);
     C2D_SpriteSetScale(&sprite,clipped_w/sprite.params.pos.w,
                              clipped_h/sprite.params.pos.h);
     C2D_SpriteSetDepth(&sprite,road_depth);
     if(!C2D_DrawSprite(&sprite))return false;
    }
  }
 }
 return roads_drawn;
}
bool Renderer::draw_source_sprite_fitted(std::uint16_t object,std::uint16_t animation,std::uint32_t frame,float x,float y,float width,float height,float depth) noexcept {
 if(width<=0.0f||height<=0.0f) return false;
 const auto key=std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{object,animation,frame};
 const auto* begin=stage1::kSpriteFrames;
 const auto* end=begin+stage1::kSpriteFrameCount;
 const auto* selected=std::lower_bound(begin,end,key,[](const stage1::SpriteFrameDef& item,const auto& wanted){
  return std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{item.object,item.animation,item.frame}<wanted;
 });
 if(selected==end||selected->object!=object||selected->animation!=animation||selected->frame!=frame ||
    selected->width<=0.0f||selected->height<=0.0f) return false;
 const float sx=width/selected->width;
 const float sy=height/selected->height;
 return draw_source_sprite(object,animation,frame,
    x+selected->origin_x*width,y+selected->origin_y*height,sx,sy,depth);
}
bool Renderer::draw_source_animation(std::uint16_t object,const char* animation_name,
                                      std::uint32_t frame,float x,float y,
                                      float sx,float sy,float depth,float rotation,bool outline) noexcept {
 if(!animation_name||!animation_name[0]) return false;
 const auto* begin=stage1::kSpriteAnimations;
 const auto* end=begin+stage1::kSpriteAnimationCount;
 const auto* found=std::lower_bound(begin,end,object,
  [](const stage1::SpriteAnimationDef& item,std::uint16_t wanted){return item.object<wanted;});
 for(;found!=end&&found->object==object;++found){
  if(::strcasecmp(found->name,animation_name)!=0) continue;
  if(found->frame_count==0) return false;
  const std::uint32_t selected=found->loop?frame%found->frame_count:
      std::min(frame,found->frame_count-1);
  return draw_source_sprite(object,found->animation,selected,x,y,sx,sy,depth,rotation,1.0f,1.0f,outline);
 }
 return false;
}
bool Renderer::draw_item_icon(std::uint16_t item_id,float center_x,float center_y,
                              float max_width,float max_height,float depth,
                              float rotation) noexcept {
 std::size_t first=0, last=items::kItemSpriteDefinitionCount;
 while(first<last) {
  const std::size_t middle=first+(last-first)/2;
  if(items::kItemSpriteDefinitions[middle].item_id<item_id) first=middle+1;
  else last=middle;
 }
 if(first>=items::kItemSpriteDefinitionCount ||
    items::kItemSpriteDefinitions[first].item_id!=item_id) return false;
 const std::uint16_t object=items::kItemSpriteDefinitions[first].object;
 const auto key=std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{object,0,0};
 const auto* begin=stage1::kSpriteFrames;
 const auto* end=begin+stage1::kSpriteFrameCount;
 const auto* frame=std::lower_bound(begin,end,key,[](const stage1::SpriteFrameDef& item,const auto& wanted){
  return std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{item.object,item.animation,item.frame}<wanted;
 });
 // Prefer the original object's animation 0/frame 0 icon. Some source
 // objects start at a nonzero animation: use their first real source frame
 // instead of displaying a blank item cell. Never substitute another object.
 if(frame==end||frame->object!=object||frame->animation!=0||frame->frame!=0 ||
    frame->width<=0.0f||frame->height<=0.0f) {
   frame=std::lower_bound(begin,end,object,
       [](const stage1::SpriteFrameDef& candidate,std::uint16_t wanted) {
         return candidate.object<wanted;
       });
   while(frame!=end && frame->object==object &&
         (frame->width<=0.0f||frame->height<=0.0f)) ++frame;
   if(frame==end||frame->object!=object) return false;
 }
 if(max_width<=0.0f||max_height<=0.0f) return false;
 // The source gui_firearm slot rotates horizontal rifle artwork by 90°;
 // use rotated frame bounds when fitting, not a squeezed/warped sprite.
 const bool quarter_turn=std::fabs(std::fabs(rotation)-1.5707963268f)<0.01f;
 const float natural_w=quarter_turn?frame->height:frame->width;
 const float natural_h=quarter_turn?frame->width:frame->height;
 const float scale=std::min(max_width/natural_w,max_height/natural_h);
 return draw_source_sprite(object,frame->animation,frame->frame,
                           center_x,center_y,scale,scale,depth,rotation);
}
bool Engine::initialize() noexcept {
 gfxInitDefault();
 if(romfsInit()!=0){ gfxExit(); return false; }
 if(!renderer_.initialize()){ romfsExit(); gfxExit(); return false; }
 previous_time_=osGetTime(); initialized_=true; return true;
}
bool Engine::set_scene(Scene* next) noexcept {
 if(!initialized_||next==scene_) return next==scene_;
 if(scene_) scene_->on_exit(); scene_=next; if(scene_) scene_->on_enter(); return true;
}
void Engine::poll_input() noexcept {
 hidScanInput(); circlePosition cp{}; touchPosition tp{};
 const u32 held=hidKeysHeld(); const bool touching=(held&KEY_TOUCH)!=0;
 hidCircleRead(&cp); if(touching) hidTouchRead(&tp);
 input_.held=held; input_.pressed=held&~previous_held_; input_.released=previous_held_&~held;
 input_.circle=cp; input_.touch=tp; input_.touching=touching; previous_held_=held;
 if(input_.pressed & KEY_SELECT) performance_overlay_=!performance_overlay_;
}
void Engine::update(float dt) noexcept {
 // Track measured elapsed time during slower 3DS frames instead of
 // discarding it. Limit only exceptional emulator pauses (250 ms).
 // The player collision integrator sub-steps large movements.
 measured_frame_seconds_=std::max(0.0f,dt);
 delta_seconds_=std::clamp(dt,0.0f,0.25f);
 if(scene_) scene_->update(input_,delta_seconds_);
}
void Engine::render() noexcept {
 renderer_.load_debug_overlay(frame_number_,delta_seconds_,input_,scene_!=nullptr);
 renderer_.begin_frame();
 renderer_.begin_screen(GFX_TOP);
 if(scene_) scene_->render(renderer_);
 if(performance_overlay_){
  // SELECT toggles real emulator frame-time + asset reload diagnostics.
  // A high reload count identifies thrashing of the 32 resident sprite
  // sheets; frame time distinguishes CPU/GPU slowdown from move speed.
  renderer_.reset_camera();
  char metrics[128]{};
  const float fps=measured_frame_seconds_>0.0f?1.0f/measured_frame_seconds_:0.0f;
  std::snprintf(metrics,sizeof(metrics),"FPS %.1f DT %.0fms TEX LOADS %lu DROPS %lu",
    static_cast<double>(fps),static_cast<double>(measured_frame_seconds_*1000.0f),
    static_cast<unsigned long>(renderer_.sprite_cache_misses()),
    static_cast<unsigned long>(renderer_.sprite_cache_failed_draws()));
  // A screenshot showing DROPS=0 does NOT prove every source world
  // sprite was spawned. Distinguish nearby source placements, actual
  // render submissions and unmapped original sprite frames.
  renderer_.draw_solid_rect(0.0f,0.0f,400.0f,31.0f,
                            C2D_Color32(14,18,15,238),0.88f);
  renderer_.draw_text(metrics,4.0f,2.0f,0.37f);
  char source_metrics[128]{};
  std::snprintf(source_metrics,sizeof(source_metrics),
      "SOURCE WORLD %lu PLACED %lu DRAWN %lu BADFRAME %lu",
      static_cast<unsigned long>(renderer_.world_sprite_candidates()),
      static_cast<unsigned long>(renderer_.world_sprite_requested()),
      static_cast<unsigned long>(renderer_.world_sprite_rendered()),
      static_cast<unsigned long>(renderer_.world_sprite_invalid()));
  renderer_.draw_text(source_metrics,4.0f,17.0f,0.28f);
 }
 // The bottom screen is reserved for diagnostics and touch UI; never
 // redraw the top-screen world here.
 renderer_.reset_camera();
 renderer_.begin_screen(GFX_BOTTOM);
 if(scene_) scene_->render_bottom(renderer_);
 renderer_.end_frame();
}
void Engine::run() noexcept {
 if(!initialized_) return;
 while(aptMainLoop()){
  poll_input();
  const u64 now=osGetTime();
  const float dt=static_cast<float>(now-previous_time_)/1000.0f;
  previous_time_=now; update(dt); render(); ++frame_number_;
 }
}
void Engine::shutdown() noexcept {
 if(!initialized_) return;
 if(scene_) scene_->on_exit(); scene_=nullptr;
 renderer_.shutdown(); romfsExit(); gfxExit(); initialized_=false;
}
}
