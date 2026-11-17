#pragma once

#include <cstdint>

namespace minidayz::items {

// The repository-root Game_events -> Rain event tree is driven by the
// Construct Timer("Rain") callback. The native day/night clock is NOT this
// timer, and must never be substituted for it. Only an interpreter that
// verified the original ancestor and branch conditions can call dispatch.
struct SourceRainEventState {
    bool active{};
    std::uint64_t last_event_uid{};

    bool dispatch(std::uint64_t source_event_uid,
                  bool original_rain_timer_fired,
                  bool original_branch_guards_satisfied) noexcept {
        if (!original_rain_timer_fired || !original_branch_guards_satisfied)
            return false;
        switch (source_event_uid) {
        // Source group 7113031514534401, nested timer event
        // 1531255057347819, stop event 448001804847237:
        // IF Rain == 1 -> SET Rain = 0.
        case 448001804847237ULL:
            if (!active) return false;
            active = false;
            break;
        // Source start event 609331584746158:
        // inRaid != 1, rains_level branch, Rain == 0 and original RNG
        // choose(1,2,3,4)>1. All those conditions must be checked by caller.
        case 609331584746158ULL:
            if (active) return false;
            active = true;
            break;
        default:
            return false;
        }
        last_event_uid=source_event_uid;
        return true;
    }
};

} // namespace minidayz::items
