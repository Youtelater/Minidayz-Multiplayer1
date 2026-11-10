#include "minidayz/engine.hpp"
#include <algorithm>

namespace minidayz {
bool Renderer::initialize() noexcept {
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) return false;
 if(!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)){ C3D_Fini(); return false; }
 C2D_Prepare(); top_=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT); bottom_=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
 if(!top_||!bottom_){ shutdown(); return false; } initialized_=true; return true;
}
void Renderer::shutdown() noexcept { unload_all(); if(initialized_||top_||bottom_){ C2D_Fini(); C3D_Fini(); } top_=bottom_=nullptr; initialized_=false; }
void Renderer::begin_frame() noexcept { C3D_FrameBegin(C3D_FRAME_SYNCDRAW); C2D_TargetClear(top_,C2D_Color32(12,16,18,255)); C2D_TargetClear(bottom_,C2D_Color32(18,22,24,255)); }
void Renderer::begin_screen(gfxScreen_t screen) noexcept { C2D_SceneBegin(screen==GFX_TOP?top_:bottom_); C2D_ViewReset(); C2D_ViewTranslate(-camera_x_,-camera_y_); }
void Renderer::end_frame() noexcept { C3D_FrameEnd(0); }
bool Renderer::load_sheet(std::size_t slot,const char* path) noexcept {
 if(!initialized_||slot>=kMaxSheets||!path||!path[0]) return false;
 C2D_SpriteSheet fresh=C2D_SpriteSheetLoad(path); if(!fresh) return false;
 if(sheets_[slot]) C2D_SpriteSheetFree(sheets_[slot]); sheets_[slot]=fresh; return true;
}
void Renderer::unload_sheet(std::size_t slot) noexcept { if(slot>=kMaxSheets||!sheets_[slot]) return; C2D_SpriteSheetFree(sheets_[slot]); sheets_[slot]=nullptr; }
void Renderer::unload_all() noexcept { for(std::size_t i=0;i<kMaxSheets;++i) unload_sheet(i); }
bool Renderer::draw_sprite(std::size_t slot,std::size_t image,float x,float y,float sx,float sy,float rotation,float depth) noexcept {
 if(slot>=kMaxSheets||!sheets_[slot]||image>=C2D_SpriteSheetCount(sheets_[slot])) return false;
 C2D_Sprite sprite{}; C2D_SpriteFromSheet(&sprite,sheets_[slot],image); C2D_SpriteSetPos(&sprite,x,y);
 C2D_SpriteSetScale(&sprite,sx,sy); C2D_SpriteSetRotation(&sprite,rotation); C2D_SpriteSetDepth(&sprite,depth); return C2D_DrawSprite(&sprite);
}
bool Engine::initialize() noexcept { gfxInitDefault(); if(romfsInit()!=0){ gfxExit(); return false; }
 if(!renderer_.initialize()){ romfsExit(); gfxExit(); return false; } previous_time_=osGetTime(); initialized_=true; return true; }
bool Engine::set_scene(Scene* next) noexcept { if(!initialized_||next==scene_) return next==scene_; if(scene_) scene_->on_exit(); scene_=next; if(scene_) scene_->on_enter(); return true; }
void Engine::poll_input() noexcept { hidScanInput(); circlePosition cp{}; touchPosition tp{}; const u32 held=hidKeysHeld(); const bool touching=(held&KEY_TOUCH)!=0;
 hidCircleRead(&cp); if(touching) hidTouchRead(&tp); input_.held=held; input_.pressed=held&~previous_held_; input_.released=previous_held_&~held;
 input_.circle=cp; input_.touch=tp; input_.touching=touching; previous_held_=held; }
void Engine::update(float dt) noexcept { delta_seconds_=std::min(std::max(dt,0.0f),0.05f); if(scene_) scene_->update(input_,delta_seconds_); }
void Engine::render() noexcept { renderer_.begin_frame(); renderer_.begin_screen(GFX_TOP); if(scene_) scene_->render(renderer_);
 renderer_.begin_screen(GFX_BOTTOM); if(scene_) scene_->render(renderer_); renderer_.end_frame(); }
void Engine::run() noexcept { if(!initialized_) return; while(aptMainLoop()){ poll_input(); if(input_.pressed&KEY_START) break;
 const u64 now=osGetTime(); const float dt=static_cast<float>(now-previous_time_)/1000.0f; previous_time_=now; update(dt); render(); ++frame_number_; } }
void Engine::shutdown() noexcept { if(!initialized_) return; if(scene_) scene_->on_exit(); scene_=nullptr; renderer_.shutdown(); romfsExit(); gfxExit(); initialized_=false; }
}
