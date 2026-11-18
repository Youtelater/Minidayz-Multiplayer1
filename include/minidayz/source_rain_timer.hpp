#pragma once

#include "minidayz/source_rain_event.hpp"
#include <cmath>
#include <cstdint>

namespace minidayz::items {

// Original root Game_events Start events, event 8520771380755414:
// Timer.Start(LOOT_RESPAWN_TIME=1480, repeat=1, "Check_loot")
// Timer.Start(Rain_speed=360, repeat=1, "Rain").
// Only the "Rain" timer is handled here; loot events have different guards.
struct SourceRainTimerSnapshot {
    SourceRainEventState rain{};
    float elapsed_seconds{};
    std::uint32_t timer_firings{};
};

class SourceRainTimer {
public:
    static constexpr float kOriginalRainSeconds = 360.0f;
    const SourceRainTimerSnapshot& snapshot() const noexcept { return state_; }
    bool active() const noexcept { return state_.rain.active; }
    void reset() noexcept { state_ = {}; }

    bool restore(const SourceRainTimerSnapshot& saved) noexcept {
        if (!std::isfinite(saved.elapsed_seconds) ||
            saved.elapsed_seconds < 0.0f ||
            saved.elapsed_seconds >= kOriginalRainSeconds ||
            (saved.rain.last_event_uid != 0 &&
             saved.rain.last_event_uid != 609331584746158ULL &&
             saved.rain.last_event_uid != 448001804847237ULL))
            return false;
        // Previous state must be compatible with the original last event.
        if (saved.rain.last_event_uid==609331584746158ULL &&
            !saved.rain.active) return false;
        if (saved.rain.last_event_uid==448001804847237ULL &&
            saved.rain.active) return false;
        if (saved.rain.last_event_uid==0 && saved.rain.active) return false;
        state_=saved;
        return true;
    }

    // Draw 1..4 is the result of the source choose(1,2,3,4), supplied by
    // the outer game RNG. The caller independently checks current original
    // global rains_level and inRaid from the current world/raid mode.
    // A single update advances elapsed time and performs at most one timer
    // firing, because the 3DS frame loop caps delta to avoid giant jumps.
    bool update(float dt, std::uint8_t rains_level, bool in_raid,
                std::uint8_t choice_1_to_4) noexcept {
        if (!std::isfinite(dt) || dt < 0.0f || dt > 1.0f ||
            rains_level > 3 || choice_1_to_4 < 1 ||
            choice_1_to_4 > 4) return false;
        const float next = state_.elapsed_seconds + dt;
        if (next < kOriginalRainSeconds) {
            state_.elapsed_seconds=next;
            return true;
        }
        state_.elapsed_seconds=next-kOriginalRainSeconds;
        ++state_.timer_firings;
        if (rains_level==0) return true;
        if (state_.rain.active) {
            (void)state_.rain.dispatch(
                448001804847237ULL,true,true);
        } else {
            const bool original_start_guards =
                !in_raid && choice_1_to_4 > 1;
            (void)state_.rain.dispatch(
                609331584746158ULL,true,original_start_guards);
        }
        return true;
    }

private:
    SourceRainTimerSnapshot state_{};
};

} // namespace minidayz::items
