#pragma once
#include <3ds.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz {

// The original Construct drop actions choose drop1/drop2; successful
// crafting uses crafting.wav. All samples come from repository romfs/wav.
class Stage2Audio {
public:
    bool initialize() noexcept;
    void play_crafting() noexcept;
    void play_item_drop() noexcept;
    void play_equipment_drop() noexcept;
    void play_backpack_zip() noexcept;
    void shutdown() noexcept;
    ~Stage2Audio() { shutdown(); }
    Stage2Audio() = default;
    Stage2Audio(const Stage2Audio&) = delete;
    Stage2Audio& operator=(const Stage2Audio&) = delete;
private:
    struct Clip {
        void* pcm{};
        std::size_t bytes{};
        std::uint32_t frames{};
        std::uint32_t sample_rate{};
        std::uint16_t channels{};
    };
    static constexpr std::size_t kClipCount = 6;
    bool load_wav(const char* path, Clip& destination) noexcept;
    void play(std::size_t clip) noexcept;
    std::array<Clip,kClipCount> clips_{};
    ndspWaveBuf wave_{};
    bool ndsp_ready_{};
    unsigned drop_serial_{};
    unsigned gear_drop_serial_{};
};

} // namespace minidayz
