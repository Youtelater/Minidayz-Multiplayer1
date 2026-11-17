#include "minidayz/stage2_audio.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace minidayz {
namespace {
constexpr std::size_t kMaximumClipBytes = 4 * 1024 * 1024;

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
    constexpr const char* paths[kClipCount] = {
        "romfs:/wav/crafting.wav",
        "romfs:/wav/drop1.wav",
        "romfs:/wav/drop2.wav"
    };
    bool any = false;
    for (std::size_t i=0;i<kClipCount;++i)
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
    ndspChnWaveBufClear(0);
    ndspChnReset(0);
    ndspChnSetRate(0, static_cast<float>(clip.sample_rate));
    ndspChnSetFormat(0, clip.channels==1
        ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);
    float mix[12]{};
    mix[0]=1.0f;
    mix[1]=1.0f;
    ndspChnSetMix(0,mix);
    wave_={};
    wave_.data_vaddr=clip.pcm;
    wave_.nsamples=clip.frames;
    wave_.looping=false;
    ndspChnWaveBufAdd(0,&wave_);
}

void Stage2Audio::play_crafting() noexcept {
    play(0);
}

void Stage2Audio::play_item_drop() noexcept {
    // Original source chooses between drop1 and drop2. Alternate the two
    // existing clips without inventing a replacement sound.
    play(1u + (drop_serial_++ % 2u));
}

void Stage2Audio::shutdown() noexcept {
    if (ndsp_ready_) {
        ndspChnWaveBufClear(0);
        ndspChnReset(0);
        ndspExit();
        ndsp_ready_ = false;
    }
    for (auto& clip:clips_) {
        if (clip.pcm) linearFree(clip.pcm);
        clip={};
    }
    wave_={};
    drop_serial_=0;
}
}
