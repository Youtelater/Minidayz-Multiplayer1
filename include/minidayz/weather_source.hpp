#pragma once
#include "minidayz/generated/weather_cold_catalog.hpp"
#include "minidayz/player.hpp"
#include <cmath>

namespace minidayz::items {

inline const SourceWeatherCold& source_weather_cold(
    minidayz::SurvivalDifficulty difficulty) noexcept {
    switch (difficulty) {
    case minidayz::SurvivalDifficulty::novice: return kSourceWeatherCold[0];
    case minidayz::SurvivalDifficulty::regular: return kSourceWeatherCold[1];
    case minidayz::SurvivalDifficulty::veteran: return kSourceWeatherCold[2];
    }
    return kSourceWeatherCold[1];
}
// These are the exact original difficulty constants, not a weather timer.
// A translated Construct event must decide when each weather state begins.
enum class SourceColdPhase : std::uint8_t { day, night, rain };

inline float source_weather_cold_for_phase(
    minidayz::SurvivalDifficulty difficulty,
    SourceColdPhase phase) noexcept {
    const auto& source = source_weather_cold(difficulty);
    switch (phase) {
    case SourceColdPhase::day: return source.day;
    case SourceColdPhase::night: return source.night;
    case SourceColdPhase::rain: return source.rain;
    }
    return source.day;
}

// The repository-root Update_Stats event has distinct mutually guarded
// Player_temperature writes. For Day_mode it subtracts clothing warmth
// (UID 4824763499434787). Night (UID 1386894733047937) assigns Night_cold
// directly. Rain branches (UIDs 110661674372728, 551577037849239,
// 927448526676784, 629017975270922) assign Rain_cold / 2 or Rain_cold.
// This function deliberately does NOT decide when those guards are true.
enum class SourceRainColdBranch : std::uint8_t { full, half };

inline bool source_weather_temperature(
    minidayz::SurvivalDifficulty difficulty, SourceColdPhase phase,
    float clothing_warmth, SourceRainColdBranch rain_branch,
    float& temperature) noexcept {
    if (!std::isfinite(clothing_warmth)) return false;
    const auto& source = source_weather_cold(difficulty);
    switch (phase) {
    case SourceColdPhase::day:
        temperature = source.day - clothing_warmth;
        return true;
    case SourceColdPhase::night:
        temperature = source.night;
        return true;
    case SourceColdPhase::rain:
        temperature = rain_branch == SourceRainColdBranch::half
            ? source.rain / 2.0f : source.rain;
        return true;
    }
    return false;
}

} // namespace minidayz::items
