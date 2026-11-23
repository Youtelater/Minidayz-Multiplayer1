#pragma once
#include <3ds.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include "minidayz/terrain_surface.hpp"

namespace minidayz {

enum class OriginalWorldSound : std::uint8_t {
    chop1, chop2, chop3, chop4, treefall02, treefall03,
    doorOpen, doorClose, doorMetalOpen, doorMetalClose,
    carTrunkOpen
};

// The original Construct drop actions choose drop1/drop2; successful
// crafting uses crafting.wav. All samples come from repository romfs/wav.
class Stage2Audio {
public:
    bool initialize() noexcept;
    void set_muted(bool value) noexcept;
    bool muted() const noexcept { return muted_; }
    bool audio_ready() const noexcept { return ndsp_ready_ || csnd_ready_; }
    bool using_csnd() const noexcept { return csnd_ready_; }
    bool ambience_loaded() const noexcept { return clips_[12].pcm != nullptr; }
    // Poll the hardware/emulator channel separately from the IPC request
    // result. Some older emulators acknowledge CSND writes without mixing
    // any audio into their output.
    void poll_ambience_playback() noexcept;
    const char* ambience_status() const noexcept {
        if (!audio_ready()) return "AUDIO DEVICE : UNAVAILABLE";
        if (!ambience_loaded()) return "AMBIENCE WAV : NOT LOADED";
        if (!ambient_play_attempted_) return "AMBIENCE : NOT STARTED";
        if (!ambient_play_ok_) return "AMBIENCE : PLAY ERROR";
        if (csnd_ready_) {
            if (ambient_csnd_state_==1) return "CSND : CHANNEL ACTIVE";
            if (ambient_csnd_state_==0) return "CSND : CHANNEL IDLE";
            if (ambient_csnd_state_==-2) return "CSND : STATE UNSUPPORTED";
            return "CSND : CHECKING CHANNEL";
        }
        return "NDSP : PLAY REQUEST OK";
    }
    void play_crafting() noexcept;
    void play_item_drop() noexcept;
    void play_equipment_drop() noexcept;
    void play_backpack_zip() noexcept;
    void play_item_pickup() noexcept;
    // Play the exact source-selected event sound, on a dedicated world voice.
    // The event executor must choose the cue only after original conditions.
    void play_original_world_sound(OriginalWorldSound cue) noexcept;
    void play_footstep(terrain::FootstepSurface surface) noexcept;
    void start_menu_ambience() noexcept;
    void set_environment_ambience(bool night,bool winter=false) noexcept;
    void stop_environment_ambience() noexcept;
    void start_rain_loop(bool blizzard=false) noexcept;
    void stop_rain_loop() noexcept;
    // Original House_transparenting mutes outdoor rain and unmutes
    // rain_roof when PlayerBase enters wall-zone 346.
    void set_rain_sheltered(bool sheltered) noexcept;
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
    static constexpr std::size_t kClipCount = 15;
    bool load_wav(const char* path, Clip& destination) noexcept;
    void play(std::size_t clip) noexcept;
    void stop_voice(std::size_t clip) noexcept;
    void set_ambient_gain(float gain) noexcept;
    std::array<Clip,kClipCount> clips_{};
    // Independent NDSP voices prevent a pickup/drop from cutting off an
    // in-progress crafting or zipper sound. Wave buffers must stay alive
    // for the entire native playback, not just the play() stack frame.
    std::array<ndspWaveBuf,kClipCount> waves_{};
    bool ndsp_ready_{};
    bool csnd_ready_{};
    std::array<int,kClipCount> csnd_channels_{};
    bool muted_{};
    unsigned drop_serial_{};
    unsigned gear_drop_serial_{};
    unsigned pickup_serial_{};
    unsigned step_serial_{};
    std::array<int,4> loaded_step_surfaces_{{-1,-1,-1,-1}};
    int loaded_pickup_variant_{-1};
    int ambient_variant_{-1};
    bool menu_ambience_{};
    bool ambient_play_attempted_{};
    bool ambient_play_ok_{};
    int ambient_csnd_state_{-1};
    unsigned csnd_status_frame_counter_{};
    int weather_sample_variant_{-1};
    void set_weather_sample_variant(int variant) noexcept;
    int loaded_world_cue_{-1};
    int loaded_treefall_cue_{-1};
};

} // namespace minidayz
