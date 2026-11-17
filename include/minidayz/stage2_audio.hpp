#pragma once
#include <3ds.h>
#include <cstddef>
#include <cstdint>

namespace minidayz {

// Source media name: romfs/wav/crafting.wav. A single optional native
// 3DS NDSP voice for successful crafting; never fabricates missing samples.
// Failure to access DSP or a supported PCM wave must not stop gameplay.
class Stage2Audio {
public:
    bool initialize() noexcept;
    void play_crafting() noexcept;
    void shutdown() noexcept;
    ~Stage2Audio() { shutdown(); }
    Stage2Audio() = default;
    Stage2Audio(const Stage2Audio&) = delete;
    Stage2Audio& operator=(const Stage2Audio&) = delete;
private:
    ndspWaveBuf wave_{};
    void* pcm_{};
    std::size_t bytes_{};
    std::uint32_t frames_{};
    std::uint32_t sample_rate_{};
    std::uint16_t channels_{};
    bool ndsp_ready_{};
};
}
