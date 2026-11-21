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
    // CSND accepts PCM16 mono; downmix stereo source WAV in memory, leaving
    // both the original media/ and committed romfs/wav/ files unchanged.
    const std::uint16_t out_channels = csnd_ready_ ? 1u : channels;
    const auto output_bytes = static_cast<std::size_t>(frames) * out_channels * 2;
    void* audio = linearAlloc(output_bytes);
    if (!audio) { std::fclose(file); return false; }
    bool loaded = false;
    if (bits == 16 && out_channels == channels) {
        loaded = std::fread(audio, 1, data_bytes, file) == data_bytes;
    } else {
        // Never allocate a second multi-megabyte copy of ambient.wav.
        // Read small complete-frame blocks and decode directly into the
        // DMA-accessible final buffer. This also supports 8-bit PCM WAVs.
        unsigned char raw[4096]{};
        const std::size_t frames_per_block=sizeof(raw)/block_align;
        auto* output=static_cast<std::int16_t*>(audio);
        std::size_t done=0;
        loaded=frames_per_block!=0;
        while (loaded && done<frames) {
            const std::size_t count=(frames-done<frames_per_block)
                ?frames-done:frames_per_block;
            const std::size_t nbytes=count*block_align;
            if (std::fread(raw,1,nbytes,file)!=nbytes) {
                loaded=false;
                break;
            }
            for(std::size_t i=0;i<count;++i) {
                const auto sample=[&](std::size_t ch) noexcept -> int {
                    const std::size_t at=i*block_align+ch*(bits/8);
                    return bits==8 ? (static_cast<int>(raw[at])-128)*256
                        : static_cast<int>(static_cast<std::int16_t>(le16(raw+at)));
                };
                if (out_channels==1) {
                    const int left=sample(0);
                    output[done+i]=static_cast<std::int16_t>(
                        channels==2 ? (left+sample(1))/2 : left);
                } else {
                    for(std::size_t ch=0;ch<channels;++ch)
                        output[(done+i)*channels+ch]=static_cast<std::int16_t>(sample(ch));
                }
            }
            done+=count;
        }
    }
    std::fclose(file);
    if (!loaded) { linearFree(audio); return false; }
    destination.pcm = audio;
    destination.bytes = output_bytes;
    destination.frames = frames;
    destination.sample_rate = rate;
    destination.channels = out_channels;
    DSP_FlushDataCache(destination.pcm,destination.bytes);
    return true;
}

bool Stage2Audio::initialize() noexcept {
    if (ndsp_ready_ || csnd_ready_) return true;
    if (R_SUCCEEDED(ndspInit())) {
        ndsp_ready_ = true;
    } else if (R_SUCCEEDED(csndInit())) {
        // libctru NDSP needs a DSP component that may be absent from
        // emulator/homebrew SD storage. CSND works without dspfirm.cdc.
        // Assign distinct available CSND hardware channels per clip.
        csnd_ready_ = true;
        csnd_channels_.fill(-1);
        int next=0;
        for (int ch=0;ch<CSND_NUM_CHANNELS && next<static_cast<int>(kClipCount);++ch)
            if (csndChannels & (1u<<ch))
                csnd_channels_[static_cast<std::size_t>(next++)]=ch;
        // The original 13 voices are mandatory for existing menu,
        // crafting, rain and environment sounds. The new effects voice
        // is optional on CSND implementations that expose fewer than 14
        // usable channels: do not regress all audio merely because the
        // additional world-effects voice is unavailable.
        if (next<13) {
            shutdown();
            return false;
        }
    } else {
        return false;
    }
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

void Stage2Audio::poll_ambience_playback() noexcept {
    if (!csnd_ready_ || !ambient_play_attempted_ || !ambient_play_ok_)
        return;
    // Only query every 30 UI frames, not every rendering call: each query
    // executes a synchronous CSND service command.
    if (++csnd_status_frame_counter_ < 30) return;
    csnd_status_frame_counter_=0;
    const int channel=csnd_channels_[12];
    if (channel<0) { ambient_csnd_state_=-2; return; }
    u8 playing=0;
    const Result result=csndIsPlaying(static_cast<u32>(channel),&playing);
    ambient_csnd_state_=R_FAILED(result) ? -2 : (playing ? 1 : 0);
}

void Stage2Audio::set_muted(bool value) noexcept {
    if(muted_==value)return;
    muted_=value;
    if(muted_ && audio_ready()) {
        for(std::size_t voice=0;voice<kClipCount;++voice)
            stop_voice(voice);
        ambient_variant_=-1;
        menu_ambience_=false;
        weather_sample_variant_=-1;
    }
}

void Stage2Audio::stop_voice(std::size_t which) noexcept {
    if (which>=kClipCount) return;
    if (ndsp_ready_) {
        ndspChnWaveBufClear(static_cast<int>(which));
        ndspChnReset(static_cast<int>(which));
        waves_[which]={};
    } else if (csnd_ready_ && csnd_channels_[which]>=0) {
        CSND_SetPlayStateR(static_cast<u32>(csnd_channels_[which]),0);
        (void)csndExecCmds(true);
    }
}

void Stage2Audio::set_ambient_gain(float gain) noexcept {
    if (csnd_ready_) {
        const int channel=csnd_channels_[12];
        if(channel>=0) {
            const u32 volume=CSND_VOL(gain,0.0f);
            CSND_SetVol(static_cast<u32>(channel),volume,volume);
            (void)csndExecCmds(true);
        }
    } else if (ndsp_ready_) {
        float mix[12]{};
        mix[0]=gain;
        mix[1]=gain;
        ndspChnSetMix(12,mix);
    }
}

void Stage2Audio::play(std::size_t which) noexcept {
    if (muted_ || !audio_ready() || which>=kClipCount) return;
    const auto& clip=clips_[which];
    if (!clip.pcm || clip.frames==0) return;
    if (which==12) {
        ambient_play_attempted_=false;
        ambient_play_ok_=false;
        ambient_csnd_state_=-1;
        csnd_status_frame_counter_=0;
    }
    stop_voice(which);
    if (csnd_ready_) {
        const int channel=csnd_channels_[which];
        if (channel<0) return;
        const u32 flags=SOUND_FORMAT_16BIT|SOUND_LINEAR_INTERP|
            ((which==6 || which==12)?SOUND_REPEAT:SOUND_ONE_SHOT);
        const float gain=(which==12 && menu_ambience_)?0.31622777f:1.0f;
        const Result result=csndPlaySound(channel,flags,clip.sample_rate,
            gain,0.0f,clip.pcm,clip.pcm,static_cast<u32>(clip.bytes));
        if(which==12) {
            ambient_play_attempted_=true;
            ambient_play_ok_=R_SUCCEEDED(result);
        }
        return;
    }
    const int channel = static_cast<int>(which);
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
    wave.looping=(which==6 || which==12);
    ndspChnWaveBufAdd(channel,&wave);
    if (which==12) {
        ambient_play_attempted_=true;
        ambient_play_ok_=true;
    }
}

void Stage2Audio::play_crafting() noexcept {
    play(0);
}

void Stage2Audio::play_original_world_sound(OriginalWorldSound cue) noexcept {
    // The original source chooses chop1..4, treefall_02/03, and door
    // wood/metal sounds in separate event branches. The caller must
    // evaluate that event before choosing this actual recorded WAV.
    if (muted_ || !audio_ready()) return;
    constexpr const char* files[]{
        "romfs:/wav/chop.wav",
        "romfs:/wav/chop2.wav",
        "romfs:/wav/chop3.wav",
        "romfs:/wav/chop4.wav",
        "romfs:/wav/treefall_02.wav",
        "romfs:/wav/treefall_03.wav",
        "romfs:/wav/open_door.wav",
        "romfs:/wav/close_door.wav",
        "romfs:/wav/open_door_metal.wav",
        "romfs:/wav/close_door_metal.wav"
    };
    const auto index=static_cast<std::size_t>(cue);
    if (index>=sizeof(files)/sizeof(files[0])) return;
    // Source treefall can follow a chop in the same event chain. Give it a
    // separate voice so loading the fall sample does not stop the chop.
    // When CSND exposes fewer than 15 channels, voice 14 is optional:
    // preserve the existing effects channel rather than crashing.
    const bool treefall=index==static_cast<std::size_t>(OriginalWorldSound::treefall02) ||
                        index==static_cast<std::size_t>(OriginalWorldSound::treefall03);
    const std::size_t voice=treefall ? 14u : 13u;
    if (csnd_ready_ && csnd_channels_[voice]<0) return;
    int& loaded_cue=treefall ? loaded_treefall_cue_ : loaded_world_cue_;
    if (loaded_cue!=static_cast<int>(index)) {
        stop_voice(voice);
        if(clips_[voice].pcm)linearFree(clips_[voice].pcm);
        clips_[voice]={};
        waves_[voice]={};
        loaded_cue=-1;
        if(!load_wav(files[index],clips_[voice]))return;
        loaded_cue=static_cast<int>(index);
    }
    play(voice);
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
    if (muted_ || !audio_ready()) return;
    // Original WAV sets are grouped by terrain in the Construct source.
    // Each NDSP footstep voice retains its own variant until the surface
    // actually changes, avoiding WAV reloads on every step.
    const char* prefix = nullptr;
    switch (surface) {
    case terrain::FootstepSurface::road: prefix = "road_run"; break;
    case terrain::FootstepSurface::forest: prefix = "forest_run"; break;
    case terrain::FootstepSurface::sand: prefix = "sand_run"; break;
    case terrain::FootstepSurface::snow: prefix = "snow_run"; break;
    case terrain::FootstepSurface::shallow_water: prefix = "swamp_step"; break;
    default: prefix = "run"; break;
    }
    const unsigned variant=step_serial_++ % 4u;
    const std::size_t voice=7u+variant;
    const unsigned source_variant=variant+1u;
    const auto surface_id=static_cast<int>(surface);
    if (loaded_step_surfaces_[variant]!=surface_id) {
        stop_voice(voice);
        if (clips_[voice].pcm) linearFree(clips_[voice].pcm);
        clips_[voice]={};
        waves_[voice]={};
        loaded_step_surfaces_[variant]=-1;
        char path[80]{};
        std::snprintf(path,sizeof(path),"romfs:/wav/%s_%u.wav",
                      prefix,source_variant);
        if (!load_wav(path,clips_[voice])) return;
        loaded_step_surfaces_[variant]=surface_id;
    }
    play(voice);
}

void Stage2Audio::play_item_pickup() noexcept {
    if (muted_ || !audio_ready()) return;
    const int variant=static_cast<int>(pickup_serial_++ & 1u);
    constexpr const char* paths[2]={"romfs:/wav/pick1.wav",
                                    "romfs:/wav/pick2.wav"};
    constexpr std::size_t voice=11;
    if (loaded_pickup_variant_!=variant) {
        stop_voice(voice);
        if (clips_[voice].pcm) linearFree(clips_[voice].pcm);
        clips_[voice]={};
        waves_[voice]={};
        loaded_pickup_variant_=-1;
        if (!load_wav(paths[variant],clips_[voice])) return;
        loaded_pickup_variant_=variant;
    }
    play(voice);
}

void Stage2Audio::start_menu_ambience() noexcept {
    // Original Menu_Events/Menu_init event 9948355272197826:
    // play ambient looping at -10 dB. Reuse the gameplay day-ambience
    // sample and NDSP voice so a menu->daytime game transition is seamless.
    if (!audio_ready() || muted_ || (ambient_variant_==0 && menu_ambience_)) return;
    set_environment_ambience(false,false);
    if (ambient_variant_!=0 || !clips_[12].pcm) return;
    constexpr float kOriginalMenuGain = 0.31622777f; // 10^(-10/20)
    set_ambient_gain(kOriginalMenuGain);
    menu_ambience_=true;
}

void Stage2Audio::set_environment_ambience(bool night,bool winter) noexcept {
    if (!audio_ready() || muted_) return;
    const int wanted=(winter ? 2 : 0) + (night ? 1 : 0);
    if (ambient_variant_==wanted) {
        if (menu_ambience_) {
            // Restore gameplay gain without restarting the shared loop.
            set_ambient_gain(1.0f);
            menu_ambience_=false;
        }
        return;
    }
    stop_environment_ambience();
    // Original day, night and winter loops. Original ground_lvl5 is snowy;
    // later-world autumn artwork does not use the winter sound variants.
    // Load one loop on demand to protect original 3DS linear memory.
    constexpr const char* paths[4]={
        "romfs:/wav/ambient.wav",
        "romfs:/wav/night_loop.wav",
        "romfs:/wav/md_winter_amb.wav",
        "romfs:/wav/md_winter_night_amb.wav"
    };
    const char* path=paths[wanted];
    // Do not hammer the SD/RomFS with a multi-megabyte WAV load every
    // frame if this optional ambience cannot fit in 3DS linear memory.
    ambient_variant_=wanted;
    if (!load_wav(path,clips_[12])) {
        // Keep variant as attempted to avoid retrying multi-MB loads each
        // frame, but report the missing buffer on the Options screen.
        return;
    }
    play(12);
}

void Stage2Audio::stop_environment_ambience() noexcept {
    if (audio_ready()) stop_voice(12);
    if (clips_[12].pcm) linearFree(clips_[12].pcm);
    clips_[12]={};
    waves_[12]={};
    ambient_variant_=-1;
    menu_ambience_=false;
    ambient_play_attempted_=false;
    ambient_play_ok_=false;
    ambient_csnd_state_=-1;
    csnd_status_frame_counter_=0;
    // Rain has its own NDSP voice and independent buffer lifetime. Changing
    // day/night ambience must not invalidate the active weather sample.
}

void Stage2Audio::set_weather_sample_variant(int variant) noexcept {
    if (!audio_ready() || muted_) return;
    if (weather_sample_variant_==variant && clips_[6].pcm) return;
    stop_rain_loop();
    // The original audio tag rain_roof refers to the rainroof asset
    // (root project[7]: rainroof.m4a/.ogg), not rain_roof.wav.
    const char* path=variant==1 ? "romfs:/wav/blizzard.wav" :
                     variant==2 ? "romfs:/wav/rainroof.wav" :
                                  "romfs:/wav/rain_loop.wav";
    if (!load_wav(path,clips_[6])) return;
    weather_sample_variant_=variant;
    play(6);
}

void Stage2Audio::start_rain_loop(bool blizzard) noexcept {
    set_weather_sample_variant(blizzard?1:0);
}

void Stage2Audio::set_rain_sheltered(bool sheltered) noexcept {
    // House events set muted state of two source rain tags. Use a single
    // looping 3DS weather voice and the actual recorded samples, switching
    // only on a state transition. Blizzard is a separate original cue.
    if (weather_sample_variant_==0 || weather_sample_variant_==2)
        set_weather_sample_variant(sheltered?2:0);
}

void Stage2Audio::stop_rain_loop() noexcept {
    // Stop the dedicated rain voice without interrupting crafting,
    // footsteps, backpacks or equipment audio on other channels.
    if (audio_ready()) stop_voice(6);
    if (clips_[6].pcm) linearFree(clips_[6].pcm);
    clips_[6]={};
    waves_[6]={};
    weather_sample_variant_=-1;
}

void Stage2Audio::shutdown() noexcept {
    if (ndsp_ready_) {
        for (std::size_t i=0;i<kClipCount;++i) stop_voice(i);
        ndspExit();
        ndsp_ready_ = false;
    }
    if (csnd_ready_) {
        for (std::size_t i=0;i<kClipCount;++i) stop_voice(i);
        csndExit();
        csnd_ready_ = false;
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
    loaded_step_surfaces_={{-1,-1,-1,-1}};
    loaded_pickup_variant_=-1;
    ambient_variant_=-1;
    menu_ambience_=false;
    ambient_play_attempted_=false;
    ambient_play_ok_=false;
    ambient_csnd_state_=-1;
    csnd_status_frame_counter_=0;
    weather_sample_variant_=-1;
    loaded_world_cue_=-1;
    loaded_treefall_cue_=-1;
}
}
