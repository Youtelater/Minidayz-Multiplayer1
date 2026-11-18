#include "minidayz/engine.hpp"
#include "minidayz/generated/sprite_catalog.hpp"
#include "minidayz/generated/ground_floor_source.hpp"
#include "minidayz/terrain_tiling.hpp"
#include "minidayz/terrain_level.hpp"
#include "minidayz/terrain_roads.hpp"
#include "minidayz/world.hpp"
#include "minidayz/generated/item_sprite_catalog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>
#include <tuple>

namespace minidayz {
bool Renderer::initialize() noexcept {
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) return false;
 if(!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)){ C3D_Fini(); return false; }
 C2D_Prepare();
 top_=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);
 bottom_=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
 if(!top_||!bottom_){ shutdown(); return false; }
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
 for(auto& sheet:menu_sheets_){if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}}
 if(debug_buffer_) C2D_TextBufDelete(debug_buffer_);
 if(debug_font_) C2D_FontFree(debug_font_);
 debug_buffer_=nullptr; debug_font_=nullptr; debug_ready_=false;
 if(initialized_||top_||bottom_){ C2D_Fini(); C3D_Fini(); }
 top_=bottom_=nullptr; initialized_=false;
}
void Renderer::begin_frame() noexcept {
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
 C2D_TargetClear(top_,C2D_Color32(12,16,18,255));
 C2D_TargetClear(bottom_,C2D_Color32(18,22,24,255));
}
void Renderer::begin_screen(gfxScreen_t screen) noexcept {
 C2D_SceneBegin(screen==GFX_TOP?top_:bottom_);
 C2D_ViewReset(); C2D_ViewTranslate(-camera_x_,-camera_y_);
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
 if(debug_ready_) C2D_DrawText(&debug_text_,C2D_AlignLeft,12.0f,12.0f,0.4f,0.38f,0.38f);
}
void Renderer::draw_solid_rect(float x,float y,float width,float height,u32 color) noexcept {
 if(width>0.0f&&height>0.0f) C2D_DrawRectSolid(x,y,0.5f,width,height,color);
}
void Renderer::draw_text(const char* value,float x,float y,float scale) noexcept {
 if(!debug_ready_||!value||!value[0]) return;
 C2D_TextBufClear(debug_buffer_);
 C2D_TextParse(&debug_text_,debug_buffer_,value);
 C2D_TextOptimize(&debug_text_);
 C2D_DrawText(&debug_text_,C2D_AlignLeft,x,y,0.4f,scale,scale);
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
bool Renderer::draw_sprite(std::size_t slot,std::size_t image,float x,float y,float sx,float sy,float rotation,float depth) noexcept {
 if(slot>=kMaxSheets||!sheets_[slot]||image>=C2D_SpriteSheetCount(sheets_[slot])) return false;
 C2D_Sprite sprite{}; C2D_SpriteFromSheet(&sprite,sheets_[slot],image); C2D_SpriteSetPos(&sprite,x,y);
 C2D_SpriteSetScale(&sprite,sx,sy); C2D_SpriteSetRotation(&sprite,rotation); C2D_SpriteSetDepth(&sprite,depth); return C2D_DrawSprite(&sprite);
}
bool Renderer::draw_source_sprite(std::uint16_t object,std::uint16_t animation,std::uint32_t frame,float x,float y,float sx,float sy,float depth,float rotation) noexcept {
 const auto key=std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{object,animation,frame};
 const auto* begin=stage1::kSpriteFrames;
 const auto* end=begin+stage1::kSpriteFrameCount;
 const auto* found=std::lower_bound(begin,end,key,[](const stage1::SpriteFrameDef& item,const auto& wanted){
  return std::tuple<std::uint16_t,std::uint16_t,std::uint32_t>{item.object,item.animation,item.frame}<wanted;
 });
 if(found==end||found->object!=object||found->animation!=animation||found->frame!=frame) return false;
 const auto* selected=found;
 std::size_t slot=kMaxSheets;
 for(std::size_t i=0;i<kMaxSheets;++i){
  if(sprite_cache_[i].sheet[0]&&std::strcmp(sprite_cache_[i].sheet,selected->sheet)==0){slot=i;break;}
 }
 if(slot==kMaxSheets){
  slot=next_sprite_cache_++%kMaxSheets;
  unload_sheet(slot);
  char path[128];
  const int n=std::snprintf(path,sizeof(path),"romfs:/gfx/%s.t3x",selected->sheet);
  if(n<=0||static_cast<std::size_t>(n)>=sizeof(path)||!load_sheet(slot,path)) return false;
  std::snprintf(sprite_cache_[slot].sheet,sizeof(sprite_cache_[slot].sheet),"%s",selected->sheet);
  sprite_cache_[slot].slot=slot;
 }
 const float draw_x=x-selected->origin_x*selected->width*sx;
 const float draw_y=y-selected->origin_y*selected->height*sy;
 return draw_sprite(slot,selected->image,draw_x,draw_y,
                     sx*selected->texture_scale_x,sy*selected->texture_scale_y,rotation,depth);
}
bool Renderer::draw_menu_art(std::size_t index,float x,float y,float width,float height,float depth) noexcept {
 // Original assets/gfx menu sprites, already converted into romfs/gfx.
 static constexpr const char* paths[]={
  "romfs:/gfx/main_menu_logo_sheet0.t3x",
  "romfs:/gfx/menu_btn_wood_sheet0.t3x",
  "romfs:/gfx/tiledbgmenu_city_up.t3x"
 };
 if(index>=3||width<=0.0f||height<=0.0f) return false;
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
bool Renderer::draw_original_ground_floor(float camera_x,float camera_y,float width,float height,std::uint8_t source_level) noexcept {
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
  if(valid){
   for(std::uint32_t page=0;page<ground_pages_;++page){
    char path[80]{};
    std::snprintf(path,sizeof(path),"romfs:/gfx/original_map_ground_p%03u.t3x",page);
    floor_sheets_[page]=C2D_SpriteSheetLoad(path);
    if(!floor_sheets_[page]||C2D_SpriteSheetCount(floor_sheets_[page])==0){
     valid=false;break;
    }
   }
  }
  if(valid){
   const char* const paths[7]={
    "romfs:/gfx/original_map_base_ground.t3x",
    "romfs:/gfx/original_map_base_ground_lvl2.t3x",
    "romfs:/gfx/original_map_base_ground_lvl3.t3x",
    "romfs:/gfx/original_map_base_ground_lvl4.t3x",
    "romfs:/gfx/original_map_base_ground_lvl5.t3x",
    "romfs:/gfx/original_map_base_ground_lvl6.t3x",
    "romfs:/gfx/original_map_base_ground_lvl7.t3x"
   };
   for(int i=0;i<7;++i){
    base_floor_sheets_[i]=C2D_SpriteSheetLoad(paths[i]);
    if(!base_floor_sheets_[i] ||
       C2D_SpriteSheetCount(base_floor_sheets_[i])==0){
     valid=false;break;
    }
   }
  }
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
 // Each source map level has its own original terrain image.
 const std::uint32_t theme=terrain::active_ground_texture(source_level);
 const std::uint32_t ground_object=terrain::active_ground_object(source_level);
 if(!base_floor_sheets_[theme]) return false;
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
   C2D_Sprite sprite{};
   C2D_SpriteFromSheet(&sprite,base_floor_sheets_[theme],0);
   if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f) return false;
   C2D_SpriteSetPos(&sprite,col*base_tile,row*base_tile);
   C2D_SpriteSetScale(&sprite,base_tile/sprite.params.pos.w,
                            base_tile/sprite.params.pos.h);
   C2D_SpriteSetDepth(&sprite,0.98f);
   if(!C2D_DrawSprite(&sprite)) return false;
  }
 // Preserve the exact source RLE cells at their original Map positions.
 for(const GroundMap& map:ground_maps_){
  if(map.object_id!=ground_object) continue;
  const int x0=std::max(0,static_cast<int>(std::floor((camera_x-map.x)/tile)));
  const int y0=std::max(0,static_cast<int>(std::floor((camera_y-map.y)/tile)));
  const int x1=std::min(static_cast<int>(map.columns),
              static_cast<int>(std::ceil((camera_x+width-map.x)/tile))+1);
  const int y1=std::min(static_cast<int>(map.rows),
              static_cast<int>(std::ceil((camera_y+height-map.y)/tile))+1);
  for(int row=y0;row<y1;++row) for(int col=x0;col<x1;++col){
   const std::uint32_t descriptor=map.cells[
     static_cast<std::size_t>(row)*map.columns+static_cast<std::size_t>(col)];
   if(descriptor==0xffffffffu) continue;
   const std::uint32_t page=descriptor>>16,frame=descriptor&0xffffu;
   if(page>=ground_pages_||!floor_sheets_[page]||
      frame>=C2D_SpriteSheetCount(floor_sheets_[page])) continue;
   C2D_Sprite sprite{};
   C2D_SpriteFromSheet(&sprite,floor_sheets_[page],frame);
   C2D_SpriteSetPos(&sprite,map.x+col*tile,map.y+row*tile);
   C2D_SpriteSetScale(&sprite,tile/sprite.params.pos.w,
                             tile/sprite.params.pos.h);
   C2D_SpriteSetDepth(&sprite,0.92f);
   C2D_DrawSprite(&sprite);
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
   map.cells.resize(static_cast<std::size_t>(map.columns)*map.rows);
   if(std::fread(map.cells.data(),sizeof(std::uint32_t),
                 map.cells.size(),file)!=map.cells.size()){
    valid=false;break;
   }
   detail_maps_.push_back(std::move(map));
  }
  std::fclose(file);
  for(std::size_t i=0;i<4&&valid;++i){
   char path[80]{};
   std::snprintf(path,sizeof(path),
                 "romfs:/gfx/original_map_detail_%u.t3x",
                 static_cast<unsigned>(kOriginalObjects[i]));
   detail_sheets_[i]=C2D_SpriteSheetLoad(path);
   if(!detail_sheets_[i]||C2D_SpriteSheetCount(detail_sheets_[i])==0)
    valid=false;
  }
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
  if(slot>=4||!detail_sheets_[slot])continue;
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
  const float depth=std::clamp(0.90f-0.0125f*map.layer,0.55f,0.95f);
  for(int row=y0;row<y1;++row)for(int col=x0;col<x1;++col){
   const auto frame=map.cells[static_cast<std::size_t>(row)*
                              map.columns+static_cast<std::size_t>(col)];
   if(frame==0xffffffffu||frame>=C2D_SpriteSheetCount(detail_sheets_[slot]))
    continue;
   C2D_Sprite sprite{};
   C2D_SpriteFromSheet(&sprite,detail_sheets_[slot],frame);
   if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f)continue;
   C2D_SpriteSetPos(&sprite,map.x+col*tile,map.y+row*tile);
   C2D_SpriteSetScale(&sprite,tile/sprite.params.pos.w,
                             tile/sprite.params.pos.h);
   C2D_SpriteSetDepth(&sprite,depth);
   (void)C2D_DrawSprite(&sprite);
  }
 }
 return true;
}
bool Renderer::draw_source_road_grid(const WorldGrid& grid,
                                    const WorldBounds& bounds,
                                    float camera_x,float camera_y,
                                    float view_width,float view_height,
                                    std::uint8_t source_level) noexcept {
 if(grid.width()==0||grid.height()==0||!ground_loaded_) return false;
 if(!(bounds.width>0.0f && bounds.height>0.0f)) return false;
 const std::uint32_t theme=terrain::active_ground_texture(source_level);
 if(!road_sheets_[theme]){
  char name[80]{};
  std::snprintf(name,sizeof(name),"romfs:/gfx/original_map_road_lvl%u.t3x",
                static_cast<unsigned>(theme+1));
  road_sheets_[theme]=C2D_SpriteSheetLoad(name);
 }
 if(!road_sheets_[theme] || C2D_SpriteSheetCount(road_sheets_[theme])==0)
  return false;
 const float cell_w=bounds.width/grid.width();
 const float cell_h=bounds.height/grid.height();
 const float road_width=stage1::kOriginalFloorTileSize;
 if(!(cell_w>0.0f&&cell_h>0.0f&&road_width>0.0f))return false;
 const int left=std::max(0,static_cast<int>(
       std::floor((camera_x-road_width)/cell_w))-1);
 const int right=std::min(static_cast<int>(grid.width()),
       static_cast<int>(std::ceil((camera_x+view_width+road_width)/cell_w))+1);
 const int top=std::max(0,static_cast<int>(
       std::floor((camera_y-road_width)/cell_h))-1);
 const int bottom=std::min(static_cast<int>(grid.height()),
       static_cast<int>(std::ceil((camera_y+view_height+road_width)/cell_h))+1);
 // C2D sprite depth .96 sits behind source map placements and authored
 // tilemaps (.92), but in front of the original ground tile (.98).
 // Render only clipped, camera-visible source Array road corridor pieces.
 for(int row=top;row<bottom;++row)for(int col=left;col<right;++col){
  const std::uint16_t code=grid.at(col,row);
  if(!terrain::horizontal_road(code)&&!terrain::vertical_road(code))
   continue;
  for(int direction=0;direction<2;++direction){
   const auto band=terrain::road_band(code,direction!=0,col,row,
                                     cell_w,cell_h,road_width);
   if(!terrain::intersects_view(band,camera_x,camera_y,
                                view_width,view_height))continue;
   const float x0=std::max(band.x,camera_x);
   const float x1=std::min(band.x+band.width,camera_x+view_width);
   const float y0=std::max(band.y,camera_y);
   const float y1=std::min(band.y+band.height,camera_y+view_height);
   // Quad clip by scaling a source-authored road texture to one
   // source cell. Grid codes and source art both come from Construct.
   C2D_Sprite sprite{};
   C2D_SpriteFromSheet(&sprite,road_sheets_[theme],0);
   if(sprite.params.pos.w<=0.0f||sprite.params.pos.h<=0.0f)
    return false;
   C2D_SpriteSetPos(&sprite,x0,y0);
   C2D_SpriteSetScale(&sprite,(x1-x0)/sprite.params.pos.w,
                            (y1-y0)/sprite.params.pos.h);
   C2D_SpriteSetDepth(&sprite,0.96f);
   if(!C2D_DrawSprite(&sprite))return false;
  }
 }
 return true;
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
                                      float sx,float sy,float depth,float rotation) noexcept {
 if(!animation_name||!animation_name[0]) return false;
 const auto* begin=stage1::kSpriteAnimations;
 const auto* end=begin+stage1::kSpriteAnimationCount;
 const auto* found=std::lower_bound(begin,end,object,
  [](const stage1::SpriteAnimationDef& item,std::uint16_t wanted){return item.object<wanted;});
 for(;found!=end&&found->object==object;++found){
  if(std::strcmp(found->name,animation_name)!=0) continue;
  if(found->frame_count==0) return false;
  const std::uint32_t selected=found->loop?frame%found->frame_count:
      std::min(frame,found->frame_count-1);
  return draw_source_sprite(object,found->animation,selected,x,y,sx,sy,depth,rotation);
 }
 return false;
}
bool Renderer::draw_item_icon(std::uint16_t item_id,float center_x,float center_y,
                              float max_width,float max_height) noexcept {
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
 const float scale=std::min(max_width/frame->width,max_height/frame->height);
 return draw_source_sprite(object,frame->animation,frame->frame,
                           center_x,center_y,scale,scale,0.2f);
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
}
void Engine::update(float dt) noexcept {
 delta_seconds_=std::min(std::max(dt,0.0f),0.05f);
 if(scene_) scene_->update(input_,delta_seconds_);
}
void Engine::render() noexcept {
 renderer_.load_debug_overlay(frame_number_,delta_seconds_,input_,scene_!=nullptr);
 renderer_.begin_frame();
 renderer_.begin_screen(GFX_TOP);
 if(scene_) scene_->render(renderer_);
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
