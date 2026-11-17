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

bool Stage2Audio::initialize() noexcept {
    if (ndsp_ready_) return pcm_ != nullptr;
    // The original converted ROMFS sound is used, never a placeholder.
    std::FILE* file = std::fopen("romfs:/wav/crafting.wav", "rb");
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
                        (static_cast<int>(raw[i]) - 128) << 8);
                loaded = true;
            }
            std::free(raw);
        }
    }
    std::fclose(file);
    if (!loaded || R_FAILED(ndspInit())) {
        linearFree(audio);
        return false;
    }
    ndsp_ready_ = true;
    pcm_ = audio;
    bytes_ = output_bytes;
    frames_ = frames;
    channels_ = channels;
    sample_rate_ = rate;
    ndspChnReset(0);
    ndspChnSetRate(0, static_cast<float>(rate));
    ndspChnSetFormat(0, channels == 1
        ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);
    DSP_FlushDataCache(pcm_, bytes_);
    return true;
}

void Stage2Audio::play_crafting() noexcept {
    if (!ndsp_ready_ || !pcm_ || frames_ == 0) return;
    // This channel is dedicated to the crafting clip: clear an old instance
    // before reusing the same linear-memory sample buffer.
    ndspChnWaveBufClear(0);
    wave_ = {};
    wave_.data_vaddr = pcm_;
    wave_.nsamples = frames_;
    wave_.looping = false;
    ndspChnWaveBufAdd(0, &wave_);
}

void Stage2Audio::shutdown() noexcept {
    if (ndsp_ready_) {
        ndspChnWaveBufClear(0);
        ndspChnReset(0);
        ndspExit();
        ndsp_ready_ = false;
    }
    if (pcm_) linearFree(pcm_);
    pcm_ = nullptr;
    wave_ = {};
    bytes_ = 0;
    frames_ = 0;
    channels_ = 0;
    sample_rate_ = 0;
}
}
