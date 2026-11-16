#include "minidayz/weather_source.hpp"
#include <cassert>
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
    return 0;
}
