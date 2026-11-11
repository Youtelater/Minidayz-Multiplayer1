#include "minidayz/engine.hpp"
#include "minidayz/player.hpp"
#include "minidayz/world.hpp"
#include "minidayz/stage1_data_access.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {

// Construct 2 creates a fresh random world on a new game; it does not use
// SEED_RUN as a numeric world seed. Use the 3DS clock and hardware tick as
// fresh entropy for this native session. This is a per-run seed, not a claim
// that the original browser Math.random() sequence is reproducible.
constexpr std::uint32_t kSaveMagic = 0x4d445a33u;
constexpr std::uint32_t kSaveVersion = 1u;
constexpr const char* kSavePath = "sdmc:/3ds/Minidayz/world.sav";
constexpr const char* kSaveTempPath = "sdmc:/3ds/Minidayz/world.tmp";

struct SavedGame {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{kSaveVersion};
    std::uint32_t seed{};
    minidayz::PlayerState player{};
};

bool load_saved_game(SavedGame& save) noexcept {
    std::FILE* file = std::fopen(kSavePath, "rb");
    if (!file) return false;
    const bool read_ok = std::fread(&save, sizeof(save), 1, file) == 1;
    const bool no_extra_data = std::fgetc(file) == EOF;
    std::fclose(file);
    return read_ok && no_extra_data && save.magic == kSaveMagic &&
        save.version == kSaveVersion && save.seed != 0 &&
        std::isfinite(save.player.x) && std::isfinite(save.player.y) &&
        std::isfinite(save.player.health) && std::isfinite(save.player.hunger) &&
        std::isfinite(save.player.thirst) && save.player.x >= 0.0f &&
        save.player.y >= 0.0f &&
        save.player.x <= minidayz::WorldSystem::kSourceMapWidth &&
        save.player.y <= minidayz::WorldSystem::kSourceMapHeight;
}

bool write_saved_game(std::uint32_t seed,
                      const minidayz::PlayerState& player) noexcept {
    (void)::mkdir("sdmc:/3ds", 0777);
    (void)::mkdir("sdmc:/3ds/Minidayz", 0777);
    SavedGame save{};
    save.seed = seed;
    save.player = player;
    std::FILE* file = std::fopen(kSaveTempPath, "wb");
    if (!file) return false;
    const bool written = std::fwrite(&save, sizeof(save), 1, file) == 1 &&
                         std::fflush(file) == 0;
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed) {
        (void)std::remove(kSaveTempPath);
        return false;
    }
    // POSIX rename replaces an existing destination atomically when both
    // paths are on the same filesystem. Do not remove the last good save first:
    // a power loss or failed rename must leave Continue usable.
    if (std::rename(kSaveTempPath, kSavePath) != 0) {
        (void)std::remove(kSaveTempPath);
        return false;
    }
    return true;
}

std::uint32_t make_new_world_seed() noexcept {
    const std::uint64_t wall = static_cast<std::uint64_t>(osGetTime());
    const std::uint64_t ticks = static_cast<std::uint64_t>(svcGetSystemTick());
    std::uint64_t value = wall ^ (ticks + 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    value ^= value >> 31;
    const std::uint32_t seed = static_cast<std::uint32_t>(value ^ (value >> 32));
    return seed != 0 ? seed : 0x6d696e69u;
}

class WorldPreviewScene final : public minidayz::Scene {
public:
    explicit WorldPreviewScene(const SavedGame* restored = nullptr) noexcept
        : world_seed_(restored ? restored->seed : make_new_world_seed()),
          has_saved_game_(restored != nullptr),
          player_(restored ? restored->player : make_initial_state()) {
        // Preview-only movement tuning. Source-equivalent values still need
        // to be supplied by the gameplay rules port.
        // Temporary preview movement tuning only. Original source movement
        // values are not guessed; they will replace these when the movement
        // event/ACE trace is complete.
        preview_rules_.walk_speed = 72.0f;
        preview_rules_.run_speed = 108.0f;
        preview_rules_.collision_half_width = 5.0f;
        preview_rules_.collision_half_height = 7.0f;
        preview_rules_.max_health = 100.0f;
        preview_rules_.max_hunger = 100.0f;
        preview_rules_.max_thirst = 100.0f;
        preview_rules_.min_temperature = -10.0f;
        preview_rules_.max_temperature = 10.0f;
        world_ready_ = world_.generate_from_source(world_seed_, "Map", 0);
    }

    void on_exit() noexcept override {
        if (world_ready_ && !menu_active_)
            (void)write_saved_game(world_seed_, player_.state());
    }

    void update(const minidayz::InputState& input, float dt) noexcept override {
        if (menu_active_) {
            bool continue_game = has_saved_game_ && (input.pressed & KEY_A) != 0;
            bool new_game = (input.pressed & KEY_X) != 0;
            if (input.touching) {
                const int tx = input.touch.px;
                const int ty = input.touch.py;
                if (tx >= 18 && tx <= 302 && ty >= 78 && ty <= 132)
                    continue_game = has_saved_game_;
                if (tx >= 18 && tx <= 302 && ty >= 145 && ty <= 202)
                    new_game = true;
            }
            if (new_game) start_new_game();
            else if (continue_game) menu_active_ = false;
            else if (!has_saved_game_ && (input.pressed & KEY_A))
                start_new_game();
            return;
        }
        minidayz::PlayerInput movement{};
        movement.move_x = static_cast<float>(input.circle.dx) / 156.0f;
        movement.move_y = -static_cast<float>(input.circle.dy) / 156.0f;
        if (input.held & KEY_DLEFT)  movement.move_x -= 1.0f;
        if (input.held & KEY_DRIGHT) movement.move_x += 1.0f;
        if (input.held & KEY_DUP)    movement.move_y -= 1.0f;
        if (input.held & KEY_DDOWN)  movement.move_y += 1.0f;
        movement.run = (input.held & KEY_B) != 0;
        if (input.touching) {
            const int tx = input.touch.px;
            const int ty = input.touch.py;
            // Touch-select one of the 15 backpack slots on the bottom screen.
            if (tx >= 10 && tx < 310 && ty >= 92 && ty < 224) {
                const int col = (tx - 10) / 60;
                const int row = (ty - 92) / 43;
                if (col >= 0 && col < 5 && row >= 0 && row < 3)
                    selected_inventory_slot_ = row * 5 + col;
            }
        }
        player_.update_movement(movement, dt, preview_rules_,
                                &WorldPreviewScene::blocked, this);
        // Source-derived survival rates are still zero until their original
        // event cadence is resolved; the update path is now live and ready.
        player_.update_survival(dt, preview_rules_);
        save_elapsed_ += dt;
        if (save_elapsed_ >= 2.0f) {
            save_elapsed_ = 0.0f;
            (void)write_saved_game(world_seed_, player_.state());
        }
    }

    void render_bottom(minidayz::Renderer& renderer) noexcept override {
        if (menu_active_) {
            renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f,
                                     C2D_Color32(20, 25, 23, 255));
            renderer.draw_text("WORLD SAVE", 18.0f, 20.0f, 0.55f);
            if (has_saved_game_) {
                renderer.draw_solid_rect(18.0f, 78.0f, 284.0f, 54.0f,
                                         C2D_Color32(62, 92, 66, 255));
                renderer.draw_text("CONTINUE", 92.0f, 96.0f, 0.55f);
            } else {
                renderer.draw_solid_rect(18.0f, 78.0f, 284.0f, 54.0f,
                                         C2D_Color32(55, 65, 58, 255));
                renderer.draw_text("NO SAVE FOUND", 70.0f, 96.0f, 0.45f);
            }
            renderer.draw_solid_rect(18.0f, 145.0f, 284.0f, 57.0f,
                                     C2D_Color32(110, 75, 49, 255));
            renderer.draw_text("NEW GAME", 92.0f, 165.0f, 0.55f);
            if (!world_ready_)
                renderer.draw_text("WORLD GENERATION FAILED - RETRY NEW GAME", 18.0f, 204.0f, 0.27f);
            renderer.draw_text("A: Continue   X: New Game", 18.0f, 216.0f, 0.32f);
            return;
        }
        // The lower 3DS screen is the touch inventory, not a debug panel.
        renderer.draw_solid_rect(0.0f, 0.0f, 320.0f, 240.0f, C2D_Color32(20, 25, 23, 255));
        renderer.draw_solid_rect(6.0f, 6.0f, 308.0f, 25.0f, C2D_Color32(54, 68, 55, 255));
        // Compact live player status meters. Values are normalized against
        // the preview caps above; exact source caps/rates remain under audit.
        const auto meter = [&](float x, float value, float maximum, u32 color) {
            renderer.draw_solid_rect(x, 15.0f, 68.0f, 8.0f,
                                     C2D_Color32(8, 12, 10, 255));
            const float ratio = maximum > 0.0f
                ? std::max(0.0f, std::min(1.0f, value / maximum)) : 0.0f;
            renderer.draw_solid_rect(x + 1.0f, 16.0f, 66.0f * ratio, 6.0f, color);
        };
        const auto& stats = player_.state();
        meter(10.0f, stats.health, preview_rules_.max_health,
              C2D_Color32(190, 65, 55, 255));
        meter(88.0f, stats.hunger, preview_rules_.max_hunger,
              C2D_Color32(204, 151, 65, 255));
        meter(166.0f, stats.thirst, preview_rules_.max_thirst,
              C2D_Color32(65, 145, 205, 255));
        const float temperature_ratio = (stats.temperature -
            preview_rules_.min_temperature) /
            (preview_rules_.max_temperature - preview_rules_.min_temperature);
        meter(244.0f, temperature_ratio * 100.0f, 100.0f,
              C2D_Color32(75, 180, 165, 255));
        // Equipment / quick-access row. Empty slots stay empty until the
        // item and equipment systems are connected to source definitions.
        for (int i = 0; i < 4; ++i) {
            const float x = 10.0f + i * 77.0f;
            renderer.draw_solid_rect(x, 38.0f, 70.0f, 43.0f, C2D_Color32(8, 11, 10, 255));
            renderer.draw_solid_rect(x + 2.0f, 40.0f, 66.0f, 39.0f, C2D_Color32(47, 54, 48, 255));
        }
        // 5 x 3 backpack grid, sized for the 320 x 240 touch screen.
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 5; ++col) {
                const int index = row * 5 + col;
                const float x = 10.0f + col * 60.0f;
                const float y = 92.0f + row * 43.0f;
                const bool selected = index == selected_inventory_slot_;
                renderer.draw_solid_rect(x, y, 56.0f, 39.0f,
                    selected ? C2D_Color32(225, 190, 105, 255) : C2D_Color32(5, 8, 7, 255));
                renderer.draw_solid_rect(x + 2.0f, y + 2.0f, 52.0f, 35.0f,
                    selected ? C2D_Color32(76, 76, 54, 255) : C2D_Color32(43, 49, 44, 255));
            }
        }
        renderer.draw_solid_rect(10.0f, 225.0f, 300.0f, 3.0f, C2D_Color32(75, 91, 70, 255));
    }

    void render(minidayz::Renderer& renderer) noexcept override {
        if (menu_active_) {
            renderer.reset_camera();
            renderer.draw_solid_rect(0.0f, 0.0f, 400.0f, 240.0f,
                                     C2D_Color32(24, 34, 29, 255));
            renderer.draw_solid_rect(30.0f, 42.0f, 340.0f, 3.0f,
                                     C2D_Color32(132, 150, 116, 255));
            renderer.draw_text("MINI DAYZ", 76.0f, 72.0f, 1.05f);
            renderer.draw_text("SURVIVE THE WILDERNESS", 65.0f, 124.0f, 0.42f);
            renderer.draw_text("SELECT CONTINUE OR START A NEW WORLD", 30.0f, 174.0f, 0.32f);
            return;
        }
        const auto& p = player_.state();
        constexpr float view_w = 400.0f;
        constexpr float view_h = 240.0f;
        const float camera_x = std::clamp(p.x - view_w * 0.5f, 0.0f,
            std::max(0.0f, world_.bounds().width - view_w));
        const float camera_y = std::clamp(p.y - view_h * 0.5f, 0.0f,
            std::max(0.0f, world_.bounds().height - view_h));
        renderer.set_camera(camera_x, camera_y);
        renderer.draw_solid_rect(camera_x, camera_y, view_w, view_h,
                                 C2D_Color32(35, 49, 42, 255));

        if (world_ready_) {
            // Render the actual source Map layout in world coordinates.
            // Cull placements outside the camera rectangle to keep 3DS draw
            // cost bounded instead of drawing the entire 36k x 20k layout.
            for (std::size_t i = 0; i < world_.placement_count(); ++i) {
                const auto* placement = world_.placement(i);
                if (!placement) continue;
                const float px = placement->x;
                const float py = placement->y;
                if (px < camera_x - 96.0f || px > camera_x + view_w + 96.0f ||
                    py < camera_y - 96.0f || py > camera_y + view_h + 96.0f)
                    continue;
                const float sx = placement->scale_x > 0.0f ? placement->scale_x : 1.0f;
                if (!renderer.draw_source_sprite(placement->object, 0, 0,
                        px, py, sx, sx, 0.5f,
                        placement->angle * 0.01745329251994329577f)) {
                    // Keep missing atlas frames visible as small source-position
                    // markers, without inventing replacement artwork.
                    renderer.draw_solid_rect(px - 1.0f, py - 1.0f, 2.0f, 2.0f,
                                             C2D_Color32(105, 120, 95, 255));
                }
            }


        }

        const bool sprite_drawn = p.alive &&
            renderer.draw_source_sprite(minidayz::kSourcePlayerObject,
                player_.source_animation_index(), player_.source_animation_frame(),
                p.x, p.y, 0.55f, 0.55f);
        if (!sprite_drawn) {
            renderer.draw_solid_rect(p.x - 5.0f, p.y - 5.0f, 10.0f, 12.0f,
                                     p.alive ? C2D_Color32(190, 126, 65, 255)
                                             : C2D_Color32(130, 45, 45, 255));
        }
    }

private:
    void start_new_game() noexcept {
        world_seed_ = make_new_world_seed();
        player_ = minidayz::PlayerSystem(make_initial_state());
        world_ready_ = world_.generate_from_source(world_seed_, "Map", 0);
        save_elapsed_ = 0.0f;
        has_saved_game_ = false;
        // Never enter a blank gameplay scene when source world generation
        // fails. Keep the menu available so the player can retry instead.
        menu_active_ = !world_ready_;
        if (world_ready_) (void)write_saved_game(world_seed_, player_.state());
    }

    static minidayz::PlayerState make_initial_state() noexcept {
        minidayz::PlayerState state{};
        state.x = 160.0f;
        state.y = 120.0f;
        state.health = 100.0f;
        state.hunger = 100.0f;
        state.thirst = 100.0f;
        state.alive = true;
        return state;
    }

    static bool blocked(void* context, float x, float y) noexcept {
        const auto* scene = static_cast<const WorldPreviewScene*>(context);
        if (!scene || x < 0.0f || y < 0.0f ||
            x > scene->world_.bounds().width || y > scene->world_.bounds().height)
            return true;
        const auto* map_layout = minidayz::stage1::find_layout("Map");
        if (!map_layout) return true;
        const auto map_index = static_cast<std::uint16_t>(
            map_layout - minidayz::stage1::kLayoutTable);
        for (std::size_t i = 0; i < scene->world_.feature_count(); ++i) {
            const auto* feature = scene->world_.feature(i);
            if (!feature) continue;
            bool blocks = feature->kind == minidayz::WorldFeatureKind::Bunker ||
                          feature->kind == minidayz::WorldFeatureKind::Obstacle ||
                          feature->kind == minidayz::WorldFeatureKind::Water;
            if (feature->kind == minidayz::WorldFeatureKind::Building) {
                const auto* layer = minidayz::stage1::find_layer(
                    map_index, feature->source_layer);
                // Construct's building collision is attached to the actual
                // Buildings layer, not its decorative shadows/base duplicates.
                blocks = layer && std::strcmp(layer->name, "Buildings") == 0;
            }
            if (!blocks) continue;
            const float w = feature->width > 0.0f ? feature->width : 12.0f;
            const float h = feature->height > 0.0f ? feature->height : 12.0f;
            if (x >= feature->position.x && x <= feature->position.x + w &&
                y >= feature->position.y && y <= feature->position.y + h)
                return true;
        }
        return false;
    }

    minidayz::WorldSystem world_{};
    std::uint32_t world_seed_{};
    float save_elapsed_{};
    bool world_ready_{};
    bool menu_active_{true};
    bool has_saved_game_{};
    minidayz::PlayerSystem player_;
    minidayz::PlayerRules preview_rules_{};
    int selected_inventory_slot_{};
};

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    minidayz::Engine engine;
    if (!engine.initialize()) {
        std::printf("Mini DAYZ+: engine initialization failed.\\n");
        return 1;
    }

    SavedGame restored{};
    const bool has_saved_game = load_saved_game(restored);
    WorldPreviewScene scene(has_saved_game ? &restored : nullptr);
    if (!engine.set_scene(&scene)) {
        std::printf("Mini DAYZ+: startup scene attachment failed.\\n");
        engine.shutdown();
        return 1;
    }

    engine.run();
    engine.shutdown();
    return 0;
}
