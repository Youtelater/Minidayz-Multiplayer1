#pragma once

#include "minidayz/weather_source.hpp"
#include <cstdint>

namespace minidayz::items {

// Source Update_Stats contains one daytime assignment, a guarded night
// override, and four mutually guarded rain overrides. The original
// Day_mode/Rain/collision/snowborn ancestry is checked by the CALLER.
// A fabricated elapsed-time cycle must never invoke these events.
struct SourceWeatherEventState {
    SourceColdPhase phase{SourceColdPhase::day};
    SourceRainColdBranch rain_branch{SourceRainColdBranch::full};
    std::uint64_t last_source_uid{};

    bool dispatch(std::uint64_t source_event_uid,
                  bool original_ancestry_satisfied) noexcept {
        if (!original_ancestry_satisfied) return false;
        switch(source_event_uid) {
        // Update_Stats daytime assignment: Day_cold - clothing warmth.
        case 4824763499434787ULL:
            phase = SourceColdPhase::day;
            rain_branch = SourceRainColdBranch::full;
            break;
        // Day_mode == 0 night override.
        case 1386894733047937ULL:
            phase = SourceColdPhase::night;
            rain_branch = SourceRainColdBranch::full;
            break;
        // Original rain tree: rain_cold/2 under the first two branches.
        case 110661674372728ULL:
        case 551577037849239ULL:
            phase = SourceColdPhase::rain;
            rain_branch = SourceRainColdBranch::half;
            break;
        // Original rain tree: full rain_cold under the other two branches.
        case 927448526676784ULL:
        case 629017975270922ULL:
            phase = SourceColdPhase::rain;
            rain_branch = SourceRainColdBranch::full;
            break;
        default:
            return false;
        }
        last_source_uid = source_event_uid;
        return true;
    }

    bool temperature(minidayz::SurvivalDifficulty difficulty,
                     float clothing_warmth,
                     std::uint8_t snowborn_level,
                     float& output) const noexcept {
        if (last_source_uid == 0) return false;
        return source_weather_temperature_with_snowborn(
            difficulty, phase, clothing_warmth, rain_branch,
            snowborn_level, output);
    }
};

} // namespace minidayz::items
