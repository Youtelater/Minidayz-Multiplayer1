#include "minidayz/weather_source.hpp"
#include <cassert>
#include <limits>
using namespace minidayz;
using namespace minidayz::items;
int main() {
    // Original Game_events difficulty constants: Day/Night/Rain.
    assert(source_weather_cold_for_phase(SurvivalDifficulty::novice, SourceColdPhase::day) == 6.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::novice, SourceColdPhase::night) == 1.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::novice, SourceColdPhase::rain) == 1.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::regular, SourceColdPhase::day) == 7.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::regular, SourceColdPhase::night) == 2.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::regular, SourceColdPhase::rain) == 2.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::veteran, SourceColdPhase::day) == 8.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::veteran, SourceColdPhase::night) == 3.0f);
    assert(source_weather_cold_for_phase(SurvivalDifficulty::veteran, SourceColdPhase::rain) == 3.0f);
    // The exact original Update_Stats event writes differ from the raw
    // difficulty constants. These are event results, not weather timers.
    float temperature = 0.0f;
    assert(source_weather_temperature(
        SurvivalDifficulty::regular, SourceColdPhase::day, 5.0f,
        SourceRainColdBranch::full, temperature));
    assert(temperature == 2.0f); // Day_cold 7 - clothing warmth 5
    assert(source_weather_temperature(
        SurvivalDifficulty::regular, SourceColdPhase::night, 5.0f,
        SourceRainColdBranch::full, temperature));
    assert(temperature == 2.0f); // Night_cold direct, no warmth subtraction
    assert(source_weather_temperature(
        SurvivalDifficulty::novice, SourceColdPhase::rain, 5.0f,
        SourceRainColdBranch::full, temperature));
    assert(temperature == 1.0f);
    assert(source_weather_temperature(
        SurvivalDifficulty::novice, SourceColdPhase::rain, 5.0f,
        SourceRainColdBranch::half, temperature));
    assert(temperature == 0.5f);
    assert(!source_weather_temperature(
        SurvivalDifficulty::novice, SourceColdPhase::day,
        std::numeric_limits<float>::quiet_NaN(),
        SourceRainColdBranch::half, temperature));
    assert(temperature == 0.5f);
    return 0;
}
