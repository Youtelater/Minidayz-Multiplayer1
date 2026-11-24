#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace minidayz::items {

// Reproduces Game_events -> Time cyles and weather (source event
// 3025805513527444): Every 1.5s Timer_ALL += 1; if >1440 set to 0.
// Map generation CurrentLevel=0 starts at Timer_ALL=361 (UID 159730919035798).
// Timer_Hours=floor(Timer_ALL/60), Timer_Minutes=Timer_ALL-Hours*60.
// The source Day&Night branches set Day_mode=0 during hours 0..5
// (UID 607253053633264), and Day_mode=1 at 6..23
// (UID 650766536843728), provided inBunker=0.
struct SourceDayNightSnapshot {
    std::uint16_t timer_all{361};
    float elapsed_seconds{};
    bool day_mode{true};
};

// Root Game_events -> Time cyles and weather / Day&Night:
// 8443703214790502 midnight: 60 + abs(0-Timer_Minutes)*0.65;
// 152562407051636 hours1..4: 100; 5643670373586102 dawn:
// abs(60-Timer_Minutes)*1.69; 446293525979786 hour19:
// abs(0-Timer_Minutes)*1; 826094463252853 hours20..24: 60.
// This is the ordinary outdoor overlay opacity, NOT Day_mode (which
// remains daytime for survival stats at hours19..23). Equipment,
// bunker/raid and force-own-texture effects are separate overrides.
inline float source_outdoor_night_opacity(unsigned hour,unsigned minute,
                                         float night_light_addition=0.0f) noexcept {
    float percent=0.0f;
    const float m=static_cast<float>(std::min(minute,59u));
    if(hour==0)percent=60.0f+m*0.65f;
    else if(hour>=1 && hour<=4)percent=100.0f;
    else if(hour==5)percent=(60.0f-m)*1.69f;
    else if(hour==19)percent=m;
    else if(hour>=20 && hour<=24)percent=60.0f;
    return std::clamp((percent-night_light_addition)*0.01f,0.0f,1.0f);
}

class SourceDayNightClock {
public:
    static constexpr float kOriginalMinuteSeconds = 1.5f;
    static constexpr std::uint16_t kOriginalDayRollover = 1440;
    const SourceDayNightSnapshot& snapshot() const noexcept { return state_; }
    bool restore(const SourceDayNightSnapshot& candidate) noexcept {
        if (candidate.timer_all>kOriginalDayRollover ||
            !std::isfinite(candidate.elapsed_seconds) ||
            candidate.elapsed_seconds<0.0f ||
            candidate.elapsed_seconds>=kOriginalMinuteSeconds)
            return false;
        // Day_mode can legitimately differ from Timer_Hours while the
        // original inBunker guard suppresses the day/night write.
        // Therefore validate the time, not a guessed phase invariant.
        state_=candidate;
        return true;
    }
    void reset() noexcept { state_={}; }
    unsigned hour() const noexcept { return state_.timer_all/60u; }
    unsigned minute() const noexcept { return state_.timer_all%60u; }
    bool day_mode() const noexcept { return state_.day_mode; }

    bool update(float delta_seconds, bool in_bunker=false) noexcept {
        if (!std::isfinite(delta_seconds) || delta_seconds<0.0f)
            return false;
        // Keep arbitrary frame times out of integer conversion/overflow.
        if (delta_seconds>86400.0f) return false;
        const double whole = static_cast<double>(state_.elapsed_seconds) +
                             static_cast<double>(delta_seconds);
        const auto increments=static_cast<std::uint32_t>(
            std::floor(whole/kOriginalMinuteSeconds));
        const float carry=static_cast<float>(
            whole-static_cast<double>(increments)*kOriginalMinuteSeconds);
        if (increments==0) {
            state_.elapsed_seconds=carry;
            return true;
        }
        state_.timer_all=static_cast<std::uint16_t>(
            (static_cast<std::uint32_t>(state_.timer_all)+increments) %
            (kOriginalDayRollover+1u));
        state_.elapsed_seconds=carry;
        if (!in_bunker) {
            const auto hours=hour();
            if (hours<=5) state_.day_mode=false;
            else if (hours>=6 && hours<=23) state_.day_mode=true;
            // At exactly Timer_ALL=1440 Construct has hour 24; neither
            // conditional Day_mode write fires. Preserve previous state.
        }
        return true;
    }
private:
    SourceDayNightSnapshot state_{};
};

} // namespace minidayz::items
