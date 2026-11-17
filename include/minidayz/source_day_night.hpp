#pragma once
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
