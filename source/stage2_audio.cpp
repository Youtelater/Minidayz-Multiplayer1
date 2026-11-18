#include "minidayz/stage2_audio.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace minidayz {
namespace {
constexpr std::size_t kMaximumClipBytes = 6 * 1024 * 1024;

std::uint16_t le16(const unsigned char* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}
std::uint32_t le32(const unsigned char* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}
}

bool Stage2Audio::load_wav(const char* path, Clip& destination) noexcept {
    // The original converted ROMFS sound is used, never a placeholder.
    std::FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    unsigned char riff[12]{};
    if (std::fread(riff, 1, sizeof(riff), file) != sizeof(riff) ||
        std::memcmp(riff, "RIFF", 4) ||
        std::memcmp(riff + 8, "WAVE", 4)) {
        std::fclose(file);
        return false;
    }
    std::uint16_t format = 0, bits = 0, channels = 0, block_align = 0;
    std::uint32_t rate = 0, data_bytes = 0;
    long data_offset = -1;
    while (true) {
        unsigned char chunk[8]{};
        if (std::fread(chunk, 1, sizeof(chunk), file) != sizeof(chunk))
            break;
        const auto length = le32(chunk + 4);
        if (std::memcmp(chunk, "fmt ", 4) == 0) {
            unsigned char fmt[16]{};
            if (length < sizeof(fmt) || length > 4096 ||
                std::fread(fmt, 1, sizeof(fmt), file) != sizeof(fmt) ||
                std::fseek(file,
                    static_cast<long>(length - sizeof(fmt) + (length & 1)), SEEK_CUR))
                break;
            format = le16(fmt);
            channels = le16(fmt + 2);
            rate = le32(fmt + 4);
            block_align = le16(fmt + 12);
            bits = le16(fmt + 14);
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            data_bytes = length;
            data_offset = std::ftell(file);
            break;
        } else {
            if (length > static_cast<std::uint32_t>(
                    std::numeric_limits<long>::max() - 1) ||
                std::fseek(file, static_cast<long>(length + (length & 1)),
                           SEEK_CUR)) break;
        }
    }
    if (format != 1 || (channels != 1 && channels != 2) ||
        (bits != 8 && bits != 16) ||
        rate < 8000 || rate > 48000 || data_offset < 0 ||
        block_align != channels * (bits / 8) ||
        data_bytes == 0 || data_bytes > kMaximumClipBytes ||
        data_bytes % block_align != 0 ||
        std::fseek(file, data_offset, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    const auto frames = data_bytes / block_align;
    const auto output_bytes = static_cast<std::size_t>(frames) * channels * 2;
    void* audio = linearAlloc(output_bytes);
    if (!audio) {
        std::fclose(file);
        return false;
    }
    bool loaded = false;
    if (bits == 16) {
        loaded = std::fread(audio, 1, data_bytes, file) == data_bytes;
    } else {
        auto* raw = static_cast<unsigned char*>(std::malloc(data_bytes));
        if (raw) {
            if (std::fread(raw, 1, data_bytes, file) == data_bytes) {
                auto* output = static_cast<std::int16_t*>(audio);
                for (std::size_t i = 0; i < data_bytes; ++i)
                    output[i] = static_cast<std::int16_t>(
                        (static_cast<int>(raw[i]) - 128) * 256);
                loaded = true;
            }
            std::free(raw);
        }
    }
    std::fclose(file);
    if (!loaded) {
        linearFree(audio);
        return false;
    }
    destination.pcm = audio;
    destination.bytes = output_bytes;
    destination.frames = frames;
    destination.sample_rate = rate;
    destination.channels = channels;
    DSP_FlushDataCache(destination.pcm,destination.bytes);
    return true;
}

bool Stage2Audio::initialize() noexcept {
    if (ndsp_ready_) return true;
    if (R_FAILED(ndspInit())) return false;
    ndsp_ready_ = true;
    constexpr const char* paths[6] = {
        "romfs:/wav/crafting.wav",
        "romfs:/wav/drop1.wav",
        "romfs:/wav/drop2.wav",
        "romfs:/wav/zippero.wav",
        "romfs:/wav/geardrop1.wav",
        "romfs:/wav/geardrop2.wav"
    };
    // rain_loop.wav is 5 MB. Load it only when Rain actually starts so
    // the original 3DS doesn't reserve that linear memory on dry days.
    bool any = false;
    for (std::size_t i=0;i<6;++i)
        any = load_wav(paths[i],clips_[i]) || any;
    if (!any) {
        shutdown();
        return false;
    }
    return true;
}

void Stage2Audio::play(std::size_t which) noexcept {
    if (!ndsp_ready_ || which>=kClipCount) return;
    const auto& clip=clips_[which];
    if (!clip.pcm || clip.frames==0) return;
    const int channel = static_cast<int>(which);
    ndspChnWaveBufClear(channel);
    ndspChnReset(channel);
    ndspChnSetRate(channel, static_cast<float>(clip.sample_rate));
    ndspChnSetFormat(channel, clip.channels==1
        ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);
    float mix[12]{};
    mix[0]=1.0f;
    mix[1]=1.0f;
    ndspChnSetMix(channel,mix);
    auto& wave=waves_[which];
    wave={};
    wave.data_vaddr=clip.pcm;
    wave.nsamples=clip.frames;
    wave.looping=(which==6 || which==10);
    ndspChnWaveBufAdd(channel,&wave);
}

void Stage2Audio::play_crafting() noexcept {
    play(0);
}

void Stage2Audio::play_item_drop() noexcept {
    // Original source chooses between drop1 and drop2. Alternate the two
    // existing clips without inventing a replacement sound.
    play(1u + (drop_serial_++ % 2u));
}

void Stage2Audio::play_equipment_drop() noexcept {
    // Original Drop_outerwear/Drop_pants/Drop_vest/Drop_backpack use
    // choose("geardrop1","geardrop2") in the Construct audio action.
    play(4u + (gear_drop_serial_++ % 2u));
}

void Stage2Audio::play_backpack_zip() noexcept {
    // Source event 443039046724475 plays zippero for backpack activation.
    play(3);
}

void Stage2Audio::play_footstep(terrain::FootstepSurface surface) noexcept {
    if (!ndsp_ready_) return;
    // Original WAV sets are grouped by terrain in the Construct source.
    // Each NDSP footstep voice retains its own variant until the surface
    // actually changes, avoiding WAV reloads on every step.
    const char* prefix = nullptr;
    switch (surface) {
    case terrain::FootstepSurface::road: prefix = "road_run"; break;
    case terrain::FootstepSurface::forest: prefix = "forest_run"; break;
    case terrain::FootstepSurface::sand: prefix = "sand_run"; break;
    case terrain::FootstepSurface::shallow_water: prefix = "swamp_step"; break;
    default: prefix = "hard_ground"; break;
    }
    const unsigned variant=step_serial_++ & 1u;
    const std::size_t voice=7u+variant;
    const auto surface_id=static_cast<int>(surface);
    if (loaded_step_surfaces_[variant]!=surface_id) {
        ndspChnWaveBufClear(static_cast<int>(voice));
        ndspChnReset(static_cast<int>(voice));
        if (clips_[voice].pcm) linearFree(clips_[voice].pcm);
        clips_[voice]={};
        waves_[voice]={};
        loaded_step_surfaces_[variant]=-1;
        char path[80]{};
        std::snprintf(path,sizeof(path),"romfs:/wav/%s_%u.wav",
                      prefix,variant+1u);
        if (!load_wav(path,clips_[voice])) return;
        loaded_step_surfaces_[variant]=surface_id;
    }
    play(voice);
}

void Stage2Audio::play_item_pickup() noexcept {
    if (!ndsp_ready_) return;
    const int variant=static_cast<int>(pickup_serial_++ & 1u);
    constexpr const char* paths[2]={"romfs:/wav/pick1.wav",
                                    "romfs:/wav/pick2.wav"};
    constexpr std::size_t voice=9;
    if (loaded_pickup_variant_!=variant) {
        ndspChnWaveBufClear(static_cast<int>(voice));
        ndspChnReset(static_cast<int>(voice));
        if (clips_[voice].pcm) linearFree(clips_[voice].pcm);
        clips_[voice]={};
        waves_[voice]={};
        loaded_pickup_variant_=-1;
        if (!load_wav(paths[variant],clips_[voice])) return;
        loaded_pickup_variant_=variant;
    }
    play(voice);
}

void Stage2Audio::set_environment_ambience(bool night) noexcept {
    if (!ndsp_ready_) return;
    const int wanted=night ? 1 : 0;
    if (ambient_variant_==wanted) return;
    stop_environment_ambience();
    // Original Construct audio resources: ambient / night_loop.
    // Load only one loop on demand to protect original 3DS linear memory.
    const char* path=night ? "romfs:/wav/night_loop.wav"
                           : "romfs:/wav/ambient.wav";
    // Do not hammer the SD/RomFS with a multi-megabyte WAV load every
    // frame if this optional ambience cannot fit in 3DS linear memory.
    ambient_variant_=wanted;
    if (!load_wav(path,clips_[10])) return;
    play(10);
}

void Stage2Audio::stop_environment_ambience() noexcept {
    if (ndsp_ready_) {
        ndspChnWaveBufClear(10);
        ndspChnReset(10);
    }
    if (clips_[10].pcm) linearFree(clips_[10].pcm);
    clips_[10]={};
    waves_[10]={};
    ambient_variant_=-1;
}

void Stage2Audio::start_rain_loop() noexcept {
    if (!ndsp_ready_) return;
    if (!clips_[6].pcm &&
        !load_wav("romfs:/wav/rain_loop.wav",clips_[6])) return;
    // Construct Rain group 609331584746158 plays rain_loop
    // continuously while original Rain=1 (outdoors).
    play(6);
}

void Stage2Audio::stop_rain_loop() noexcept {
    // Stop the dedicated rain voice without interrupting crafting,
    // footsteps, backpacks or equipment audio on other channels.
    if (ndsp_ready_) {
        ndspChnWaveBufClear(6);
        ndspChnReset(6);
    }
    if (clips_[6].pcm) linearFree(clips_[6].pcm);
    clips_[6]={};
    waves_[6]={};
}

void Stage2Audio::shutdown() noexcept {
    if (ndsp_ready_) {
        for (std::size_t i=0;i<kClipCount;++i) {
            const int channel=static_cast<int>(i);
            ndspChnWaveBufClear(channel);
            ndspChnReset(channel);
        }
        ndspExit();
        ndsp_ready_ = false;
    }
    for (auto& clip:clips_) {
        if (clip.pcm) linearFree(clip.pcm);
        clip={};
    }
    waves_={};
    drop_serial_=0;
    gear_drop_serial_=0;
    pickup_serial_=0;
    step_serial_=0;
    loaded_step_surfaces_={{-1,-1}};
    loaded_pickup_variant_=-1;
    ambient_variant_=-1;
}
}
