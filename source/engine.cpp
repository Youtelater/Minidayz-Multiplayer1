#include "minidayz/engine.hpp"\n#include "minidayz/generated/sprite_catalog.hpp"

#include <algorithm>
#include <cstdio>\n#include <cstring>

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
bool Renderer::load_sheet(std::size_t slot,const char* path) noexcept {
 if(!initialized_||slot>=kMaxSheets||!path||!path[0]) return false;
 C2D_SpriteSheet fresh=C2D_SpriteSheetLoad(path); if(!fresh) return false;
 if(sheets_[slot]) C2D_SpriteSheetFree(sheets_[slot]); sheets_[slot]=fresh; return true;
}
void Renderer::unload_sheet(std::size_t slot) noexcept {
 if(slot>=kMaxSheets||!sheets_[slot]) return;
 C2D_SpriteSheetFree(sheets_[slot]); sheets_[slot]=nullptr;
}
void Renderer::unload_all() noexcept { for(std::size_t i=0;i<kMaxSheets;++i) unload_sheet(i); }
bool Renderer::draw_sprite(std::size_t slot,std::size_t image,float x,float y,float sx,float sy,float rotation,float depth) noexcept {
 if(slot>=kMaxSheets||!sheets_[slot]||image>=C2D_SpriteSheetCount(sheets_[slot])) return false;
 C2D_Sprite sprite{}; C2D_SpriteFromSheet(&sprite,sheets_[slot],image); C2D_SpriteSetPos(&sprite,x,y);
 C2D_SpriteSetScale(&sprite,sx,sy); C2D_SpriteSetRotation(&sprite,rotation); C2D_SpriteSetDepth(&sprite,depth); return C2D_DrawSprite(&sprite);
}
bool Renderer::draw_source_sprite(std::uint16_t object,std::uint16_t animation,std::uint32_t frame,float x,float y,float sx,float sy,float depth) noexcept {
 const stage1::SpriteFrameDef* selected=nullptr;
 for(std::size_t i=0;i<stage1::kSpriteFrameCount;++i){
  const auto& candidate=stage1::kSpriteFrames[i];
  if(candidate.object==object&&candidate.animation==animation&&candidate.frame==frame){selected=&candidate;break;}
 }
 if(!selected) return false;
 std::size_t slot=kMaxSheets;
 for(std::size_t i=0;i<kMaxSheets;++i){
  if(sprite_cache_[i].sheet[0]&&std::strcmp(sprite_cache_[i].sheet,selected->sheet)==0){slot=i;break;}
 }
 if(slot==kMaxSheets){
  slot=next_sprite_cache_++%kMaxSheets;
  unload_sheet(slot);
  char path[128];
  const int n=std::snprintf(path,sizeof(path),"romfs:/gfx/sprites/%s.t3x",selected->sheet);
  if(n<=0||static_cast<std::size_t>(n)>=sizeof(path)||!load_sheet(slot,path)) return false;
  std::snprintf(sprite_cache_[slot].sheet,sizeof(sprite_cache_[slot].sheet),"%s",selected->sheet);
  sprite_cache_[slot].slot=slot;
 }
 const float draw_x=x-selected->origin_x*selected->width*sx;
 const float draw_y=y-selected->origin_y*selected->height*sy;
 return draw_sprite(slot,selected->image,draw_x,draw_y,sx,sy,0.0f,depth);
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
 renderer_.begin_screen(GFX_BOTTOM);
 renderer_.draw_debug_overlay();
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
