#include "minidayz/engine.hpp"
#include "minidayz/player.hpp"
#include "minidayz/world.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

class WorldPreviewScene final : public minidayz::Scene {
public:
    WorldPreviewScene() noexcept
        : player_(make_initial_state()) {
        // Preview-only movement tuning. Source-equivalent values still need
        // to be supplied by the gameplay rules port.
        preview_rules_.walk_speed = 72.0f;
        preview_rules_.run_speed = 108.0f;
        world_ready_ = world_.generate_from_source(0x4d444159u, "Map", 0);
    }

    void update(const minidayz::InputState& input, float dt) noexcept override {
        minidayz::PlayerInput movement{};
        movement.move_x = static_cast<float>(input.circle.dx) / 156.0f;
        movement.move_y = -static_cast<float>(input.circle.dy) / 156.0f;
        movement.run = false;
        player_.update_movement(movement, dt, preview_rules_,
                                &WorldPreviewScene::blocked, this);
    }

    void render(minidayz::Renderer& renderer) noexcept override {
        // Render the source Map layout as a scaled overview. This deliberately
        // visualizes source placements; it does not pretend that placeholder
        // rectangles are the final original sprite/tile renderer.
        constexpr float map_x = 8.0f;
        constexpr float map_y = 8.0f;
        constexpr float map_w = 304.0f;
        constexpr float map_h = 224.0f;
        constexpr float sx = map_w / minidayz::WorldSystem::kSourceMapWidth;
        constexpr float sy = map_h / minidayz::WorldSystem::kSourceMapHeight;

        renderer.draw_solid_rect(map_x, map_y, map_w, map_h,
                                 C2D_Color32(35, 49, 42, 255));

        if (world_ready_) {
            for (std::size_t i = 0; i < world_.feature_count(); ++i) {
                const auto* feature = world_.feature(i);
                if (!feature) continue;
                u32 color = C2D_Color32(75, 91, 70, 255);
                float w = 1.2f;
                float h = 1.2f;
                switch (feature->kind) {
                case minidayz::WorldFeatureKind::Terrain:
                    color = C2D_Color32(56, 76, 59, 255);
                    break;
                case minidayz::WorldFeatureKind::Road:
                    color = C2D_Color32(145, 133, 105, 255);
                    w = 2.0f; h = 2.0f;
                    break;
                case minidayz::WorldFeatureKind::Building:
                    color = C2D_Color32(184, 166, 130, 255);
                    w = 2.0f; h = 2.0f;
                    break;
                case minidayz::WorldFeatureKind::Bunker:
                    color = C2D_Color32(196, 88, 66, 255);
                    w = 2.5f; h = 2.5f;
                    break;
                case minidayz::WorldFeatureKind::Bridge:
                    color = C2D_Color32(225, 190, 83, 255);
                    w = 2.5f; h = 2.5f;
                    break;
                case minidayz::WorldFeatureKind::SpawnPoint:
                    color = C2D_Color32(85, 190, 210, 255);
                    break;
                }
                const float px = map_x + feature->position.x * sx;
                const float py = map_y + feature->position.y * sy;
                renderer.draw_solid_rect(px, py, w, h, color);
            }

            // Display the generated POI/road grid in the lower-right corner.
            // It is a diagnostic inset until the source cell-to-world
            // transform and original location templates are ported.
            constexpr float inset_x = 222.0f;
            constexpr float inset_y = 158.0f;
            constexpr float cell_w = 5.0f;
            constexpr float cell_h = 4.0f;
            for (int y = 0; y < world_.grid().height(); ++y) {
                for (int x = 0; x < world_.grid().width(); ++x) {
                    const auto code = world_.grid().at(x, y);
                    if (code == 0) continue;
                    u32 color = C2D_Color32(170, 170, 150, 255);
                    if (code == 12 || code == 13 || code == 14)
                        color = C2D_Color32(220, 195, 130, 255);
                    else if (code == 5)
                        color = C2D_Color32(190, 85, 70, 255);
                    else if (code == 9 || code == 6)
                        color = C2D_Color32(100, 190, 110, 255);
                    else if (code == 11 || code == 15 || code == 103)
                        color = C2D_Color32(90, 170, 210, 255);
                    renderer.draw_solid_rect(inset_x + x * cell_w,
                                             inset_y + y * cell_h,
                                             cell_w - 1.0f, cell_h - 1.0f,
                                             color);
                }
            }
        } else {
            renderer.draw_solid_rect(12.0f, 12.0f, 296.0f, 8.0f,
                                     C2D_Color32(190, 55, 55, 255));
        }

        // This actor remains a controllable preview marker, not yet positioned
        // in source-world coordinates.
        const auto& p = player_.state();
        renderer.draw_solid_rect(p.x - 8.0f, p.y - 5.0f, 16.0f, 18.0f,
                                 C2D_Color32(18, 20, 18, 255));
        renderer.draw_solid_rect(p.x - 6.0f, p.y - 10.0f, 12.0f, 12.0f,
                                 C2D_Color32(205, 184, 139, 255));
        renderer.draw_solid_rect(p.x - 7.0f, p.y + 2.0f, 14.0f, 12.0f,
                                 p.alive ? C2D_Color32(190, 126, 65, 255)
                                         : C2D_Color32(130, 45, 45, 255));
        renderer.draw_solid_rect(p.x - 2.0f, p.y + 13.0f, 4.0f, 3.0f,
                                 C2D_Color32(235, 215, 150, 255));
    }

private:
    static minidayz::PlayerState make_initial_state() noexcept {
        minidayz::PlayerState state{};
        state.x = 160.0f;
        state.y = 120.0f;
        state.health = 100.0f;
        state.alive = true;
        return state;
    }

    static bool blocked(void*, float x, float y) noexcept {
        return x < 18.0f || x > 302.0f || y < 20.0f || y > 220.0f;
    }

    minidayz::WorldSystem world_{};
    bool world_ready_{};
    minidayz::PlayerSystem player_;
    minidayz::PlayerRules preview_rules_{};
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

    WorldPreviewScene scene;
    if (!engine.set_scene(&scene)) {
        std::printf("Mini DAYZ+: startup scene attachment failed.\\n");
        engine.shutdown();
        return 1;
    }

    engine.run();
    engine.shutdown();
    return 0;
}
