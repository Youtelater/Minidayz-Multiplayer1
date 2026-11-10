#pragma once

#include <3ds.h>
#include <citro2d.h>
#include <cstdint>

namespace minidayz {

struct InputState {
    u32 held{};
    u32 pressed{};
    u32 released{};
    circlePosition circle{};
    touchPosition touch{};
    bool touching{};
};

class Engine {
public:
    bool initialize() noexcept;
    void run() noexcept;
    void shutdown() noexcept;

    const InputState& input() const noexcept { return input_; }
    float delta_seconds() const noexcept { return delta_seconds_; }
    std::uint64_t frame_number() const noexcept { return frame_number_; }

private:
    void poll_input() noexcept;
    void update(float dt) noexcept;
    void render() noexcept;

    C3D_RenderTarget* top_{};
    C3D_RenderTarget* bottom_{};
    InputState input_{};
    u32 previous_held_{};
    u64 previous_time_{};
    float delta_seconds_{};
    std::uint64_t frame_number_{};
    bool initialized_{};
};

} // namespace minidayz
