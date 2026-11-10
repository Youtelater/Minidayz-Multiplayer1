#include "minidayz/engine.hpp"

#include <algorithm>

namespace minidayz {

bool Engine::initialize() noexcept {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, nullptr);

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return false;
    }
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        gfxExit();
        return false;
    }

    C2D_Prepare();
    top_ = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom_ = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!top_ || !bottom_) {
        shutdown();
        return false;
    }

    romfsInit();
    previous_time_ = osGetTime();
    initialized_ = true;
    return true;
}

void Engine::poll_input() noexcept {
    hidScanInput();
    circlePosition circle{};
    touchPosition touch{};
    hidCircleRead(&circle);
    const bool touching = (hidKeysHeld() & KEY_TOUCH) != 0;
    if (touching) hidTouchRead(&touch);

    const u32 held = hidKeysHeld();
    input_.held = held;
    input_.pressed = held & ~previous_held_;
    input_.released = previous_held_ & ~held;
    input_.circle = circle;
    input_.touch = touch;
    input_.touching = touching;
    previous_held_ = held;
}

void Engine::update(float dt) noexcept {
    // Gameplay systems plug into this deterministic per-frame update boundary.
    // Keep dt bounded so a suspended 3DS cannot cause a huge simulation step.
    delta_seconds_ = std::min(dt, 0.05f);
}

void Engine::render() noexcept {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top_, C2D_Color32(12, 16, 18, 255));
    C2D_TargetClear(bottom_, C2D_Color32(18, 22, 24, 255));

    C2D_SceneBegin(top_);
    // Scene rendering is owned by the game/rendering modules.
    C2D_SceneBegin(bottom_);

    C3D_FrameEnd(0);
}

void Engine::run() noexcept {
    if (!initialized_) return;

    while (aptMainLoop()) {
        poll_input();
        if (input_.pressed & KEY_START) break;

        const u64 now = osGetTime();
        const float dt = static_cast<float>(now - previous_time_) / 1000.0f;
        previous_time_ = now;

        update(dt);
        render();
        ++frame_number_;
    }
}

void Engine::shutdown() noexcept {
    if (initialized_ || top_ || bottom_) {
        C2D_Fini();
        C3D_Fini();
        romfsExit();
        gfxExit();
    }
    top_ = nullptr;
    bottom_ = nullptr;
    initialized_ = false;
}

} // namespace minidayz
