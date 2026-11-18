#pragma once

#include <3ds.h>
#include <citro2d.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidayz {
class WorldGrid;
struct WorldBounds;
struct InputState { u32 held{}, pressed{}, released{}; circlePosition circle{}; touchPosition touch{}; bool touching{}; };
class Renderer {
public:
 static constexpr std::size_t kMaxSheets=32;
 bool initialize() noexcept; void shutdown() noexcept;
 void begin_frame() noexcept; void begin_screen(gfxScreen_t screen) noexcept; void end_frame() noexcept;
 void load_debug_overlay(std::uint64_t frame, float delta, const InputState& input, bool scene_active) noexcept;
 void draw_debug_overlay() noexcept;
 void draw_solid_rect(float x,float y,float width,float height,u32 color) noexcept;
 void draw_text(const char* value,float x,float y,float scale=0.4f) noexcept;
 bool load_sheet(std::size_t slot,const char* romfs_path) noexcept;
 void unload_sheet(std::size_t slot) noexcept; void unload_all() noexcept;
 bool draw_sprite(std::size_t slot,std::size_t image,float x,float y,float sx=1.0f,float sy=1.0f,float rotation=0.0f,float depth=0.5f) noexcept;
 bool draw_source_sprite(std::uint16_t object,std::uint16_t animation,std::uint32_t frame,float x,float y,float scale_x=1.0f,float scale_y=1.0f,float depth=0.5f,float rotation=0.0f) noexcept;
 // Draw the original Construct sprite frame to an exact 3DS rectangle.
 // Keeps source artwork and texture scaling; only the destination changes.
 bool draw_original_ground_floor(float camera_x,float camera_y,float view_width,float view_height,std::uint8_t source_level=0,float second_half_y=1.0e30f) noexcept;
 bool draw_original_environment_tilemaps(float camera_x,float camera_y,
                                           float view_width,float view_height) noexcept;
 // Render source-derived SetTile scatter events for each generated Map cell.
 bool draw_source_procedural_environment(const class WorldSystem& world,
     float camera_x,float camera_y,float view_width,float view_height) noexcept;
 bool draw_source_road_grid(const WorldGrid& grid, const WorldBounds& bounds,
                            float camera_x,float camera_y,float view_width,float view_height,
                            std::uint8_t source_level=0) noexcept;
  bool draw_source_sprite_fitted(std::uint16_t object,std::uint16_t animation,std::uint32_t frame,float x,float y,float width,float height,float depth=0.4f) noexcept;
  bool draw_source_animation(std::uint16_t object,const char* animation_name,std::uint32_t frame,float x,float y,float scale_x=1.0f,float scale_y=1.0f,float depth=0.5f,float rotation=0.0f) noexcept;
 // Render any original assets/gfx PNG by its exact source filename.
 bool draw_original_image(const char* original_png,float x,float y,float width,float height,float depth=0.5f) noexcept;
 bool draw_menu_art(std::size_t index,float x,float y,float width,float height,float depth=0.7f) noexcept;
 bool draw_item_icon(std::uint16_t item_id,float center_x,float center_y,float max_width,float max_height) noexcept;
 void set_camera(float x,float y) noexcept { camera_x_=x; camera_y_=y; C2D_ViewReset(); C2D_ViewTranslate(-x,-y); }
 void reset_camera() noexcept { set_camera(0.0f,0.0f); }
private:
 C3D_RenderTarget* top_{}; C3D_RenderTarget* bottom_{}; C2D_SpriteSheet floor_sheets_[8]{}; C2D_SpriteSheet base_floor_sheets_[7]{}; C2D_SpriteSheet road_sheets_[7]{}; C2D_SpriteSheet detail_sheets_[4]{}; C2D_SpriteSheet menu_sheets_[3]{};
  struct GroundMap {float x{},y{};std::uint32_t columns{},rows{},object_id{};std::vector<std::uint32_t> cells;};
  struct DetailMap {
    float x{},y{};
    std::uint32_t tile_size{},columns{},rows{};
    std::uint16_t object_id{},layer{};
    std::vector<std::uint32_t> cells{};
  };
  std::vector<DetailMap> detail_maps_{};
  bool detail_load_attempted_{}; bool detail_loaded_{};
  std::vector<GroundMap> ground_maps_{};std::uint32_t ground_tile_pixels_{};std::uint32_t ground_pages_{};
  bool ground_load_attempted_{};bool ground_loaded_{}; std::uint8_t ground_active_level_{255}; C2D_SpriteSheet sheets_[kMaxSheets]{};
 struct SpriteCacheEntry { char sheet[128]{}; std::size_t slot{}; };
 SpriteCacheEntry sprite_cache_[kMaxSheets]{}; std::size_t next_sprite_cache_{};
 C2D_Font debug_font_{}; C2D_TextBuf debug_buffer_{}; C2D_Text debug_text_{};
 float camera_x_{},camera_y_{}; bool initialized_{}; bool debug_ready_{};
};
class Scene {
public:
 virtual ~Scene()=default; virtual void on_enter() noexcept {} virtual void on_exit() noexcept {}
 virtual void update(const InputState&,float) noexcept=0; virtual void render(Renderer&) noexcept=0; virtual void render_bottom(Renderer&) noexcept {}
};
class Engine {
public:
 bool initialize() noexcept; void run() noexcept; void shutdown() noexcept; bool set_scene(Scene* scene) noexcept;
 const InputState& input() const noexcept{return input_;} float delta_seconds() const noexcept{return delta_seconds_;}
 std::uint64_t frame_number() const noexcept{return frame_number_;} Renderer& renderer() noexcept{return renderer_;}
private:
 void poll_input() noexcept; void update(float dt) noexcept; void render() noexcept;
 Renderer renderer_{}; Scene* scene_{}; InputState input_{}; u32 previous_held_{}; u64 previous_time_{};
 float delta_seconds_{}; std::uint64_t frame_number_{}; bool initialized_{};
};
}
