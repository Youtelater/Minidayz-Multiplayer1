#pragma once
#include "minidayz/generated/weather_cold_catalog.hpp"
#include "minidayz/player.hpp"

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

} // namespace minidayz::items
