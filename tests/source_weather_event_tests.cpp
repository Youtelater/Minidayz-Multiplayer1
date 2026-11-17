#include "minidayz/source_weather_event.hpp"
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <limits>

using namespace minidayz;
using namespace minidayz::items;

int main() {
    SourceWeatherEventState state;
    float weather = 1000.0f;
    assert(!state.temperature(SurvivalDifficulty::regular, 3.0f, 0, weather));
    assert(weather == 1000.0f);
    assert(!state.dispatch(4824763499434787ULL, false));
    assert(state.last_source_uid == 0);
    assert(!state.dispatch(0, true));
    assert(state.dispatch(4824763499434787ULL, true));
    assert(state.phase == SourceColdPhase::day);
    assert(state.temperature(SurvivalDifficulty::regular, 3.0f, 0, weather));
    assert(weather == source_weather_cold(SurvivalDifficulty::regular).day-3.0f);
    assert(state.dispatch(1386894733047937ULL, true));
    assert(state.phase == SourceColdPhase::night);
    assert(state.temperature(SurvivalDifficulty::regular, 3.0f, 2, weather));
    assert(weather == source_weather_cold(SurvivalDifficulty::regular).night+2.0f);
    for (auto uid: {110661674372728ULL,551577037849239ULL}) {
        assert(state.dispatch(uid,true));
        assert(state.phase==SourceColdPhase::rain);
        assert(state.rain_branch==SourceRainColdBranch::half);
        assert(state.temperature(SurvivalDifficulty::regular,10.0f,0,weather));
        assert(weather==source_weather_cold(SurvivalDifficulty::regular).rain/2.0f);
    }
    for (auto uid: {927448526676784ULL,629017975270922ULL}) {
        assert(state.dispatch(uid,true));
        assert(state.phase==SourceColdPhase::rain);
        assert(state.rain_branch==SourceRainColdBranch::full);
        assert(state.temperature(SurvivalDifficulty::regular,10.0f,0,weather));
        assert(weather==source_weather_cold(SurvivalDifficulty::regular).rain);
    }
    assert(!state.dispatch(1386894733047937ULL,false));
    assert(state.last_source_uid==629017975270922ULL);
    assert(!state.temperature(SurvivalDifficulty::regular,
        std::numeric_limits<float>::quiet_NaN(),0,weather));
    assert(state.dispatch(4824763499434787ULL,true));
    assert(state.temperature(SurvivalDifficulty::regular,4.0f,0,weather));
    assert(weather==source_weather_cold(SurvivalDifficulty::regular).day-4.0f);
    return 0;
}
