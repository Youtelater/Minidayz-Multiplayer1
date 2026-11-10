#include "minidayz/engine.hpp"
#include "minidayz/player.hpp"

#include <cmath>
#include <cstdio>

namespace {

class PlayerPreviewScene final : public minidayz::Scene {
public:
    PlayerPreviewScene() noexcept
        : player_(make_initial_state()) {
        // Temporary preview tuning only. These are not claimed to be
        // source-equivalent Mini DAYZ movement constants.
        preview_rules_.walk_speed = 72.0f;
        preview_rules_.run_speed = 108.0f;
    }

    void update(const minidayz::InputState& input, float dt) noexcept override {
        // The Circle Pad is the movement control. Do not map the D-pad or
        // an arbitrary button to movement/running.
        minidayz::PlayerInput movement{};
        movement.move_x = static_cast<float>(input.circle.dx) / 156.0f;
        movement.move_y = -static_cast<float>(input.circle.dy) / 156.0f;
        movement.run = false;
        player_.update_movement(movement, dt, preview_rules_,
                                &PlayerPreviewScene::blocked, this);
    }

    void render(minidayz::Renderer& renderer) noexcept override {
        // A visible placeholder playfield, intentionally independent of
        // legacy ROMFS artwork. The real source-matched map and sprites
        // will replace this preview as those systems are integrated.
        for (int x = 8; x < 320; x += 32)
            renderer.draw_solid_rect(static_cast<float>(x), 8.0f, 1.0f, 224.0f,
                                     C2D_Color32(43, 57, 49, 255));
        for (int y = 8; y < 240; y += 32)
            renderer.draw_solid_rect(8.0f, static_cast<float>(y), 304.0f, 1.0f,
                                     C2D_Color32(43, 57, 49, 255));

        const auto& p = player_.state();
        renderer.draw_solid_rect(p.x - 8.0f, p.y - 5.0f, 16.0f, 18.0f,
                                 C2D_Color32(18, 20, 18, 255));
        renderer.draw_solid_rect(p.x - 6.0f, p.y - 10.0f, 12.0f, 12.0f,
                                 C2D_Color32(205, 184, 139, 255));
        renderer.draw_solid_rect(p.x - 7.0f, p.y + 2.0f, 14.0f, 12.0f,
                                 p.alive ? C2D_Color32(190, 126, 65, 255)
                                         : C2D_Color32(130, 45, 45, 255));
        // A bright marker makes the active player position easy to see.
        renderer.draw_solid_rect(p.x - 2.0f, p.y + 13.0f, 4.0f, 3.0f,
                                 C2D_Color32(235, 215, 150, 255));
    }

private:
    static minidayz::PlayerState make_initial_state() noexcept {
        minidayz::PlayerState state{};
        state.x = 160.0f;
        state.y = 120.0f;
        state.alive = true;
        return state;
    }

    static float axis_from_keys(u32 held, u32 positive, u32 negative) noexcept {
        const bool pos = (held & positive) != 0;
        const bool neg = (held & negative) != 0;
        return pos == neg ? 0.0f : (pos ? 1.0f : -1.0f);
    }

    static bool blocked(void*, float x, float y) noexcept {
        // Temporary preview bounds; world collision replaces these.
        return x < 18.0f || x > 302.0f || y < 20.0f || y > 220.0f;
    }

    minidayz::PlayerSystem player_;
    minidayz::PlayerRules preview_rules_{};
};

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    minidayz::Engine engine;
    if (!engine.initialize()) {
        std::printf("Mini DAYZ+: engine initialization failed.\n");
        return 1;
    }

    PlayerPreviewScene scene;
    if (!engine.set_scene(&scene)) {
        std::printf("Mini DAYZ+: startup scene attachment failed.\n");
        engine.shutdown();
        return 1;
    }

    engine.run();
    engine.shutdown();
    return 0;
}
