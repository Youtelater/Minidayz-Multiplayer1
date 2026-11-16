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
} // namespace minidayz::items
