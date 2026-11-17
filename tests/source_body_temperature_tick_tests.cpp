#include "minidayz/player.hpp"
#include <cassert>
#include <cmath>
#include <limits>

using namespace minidayz;

static PlayerSystem healthy(float warmth, float health=100.0f) {
    PlayerState state{};
    state.alive=true;
    state.x=120.0f;
    state.y=130.0f;
    state.health=health;
    state.hunger=100.0f;
    state.thirst=100.0f;
    state.temperature=warmth;
    return PlayerSystem(state);
}

int main() {
    PlayerSystem player=healthy(10.0f);
    // Original actor variable 16, not the weather exposure global.
    assert(player.apply_source_body_temperature_tick(-6.0f));
    assert(std::fabs(player.state().temperature-7.0f)<0.0001f);
    assert(player.state().health==100.0f);
    assert(player.apply_source_body_temperature_tick(-12.0f));
    assert(player.state().temperature==1.0f);
    assert(player.apply_source_body_temperature_tick(-4.0f));
    assert(player.state().temperature==0.0f);
    assert(player.state().health==99.75f);
    assert(player.apply_source_body_temperature_tick(8.0f));
    assert(player.state().temperature==4.0f);
    assert(player.state().health==99.75f);
    assert(!player.apply_source_body_temperature_tick(
        std::numeric_limits<float>::quiet_NaN()));
    assert(player.state().temperature==4.0f);
    assert(player.state().health==99.75f);
    // The original cold damage can kill, with a cold-specific reason.
    PlayerSystem terminal=healthy(0.0f,0.25f);
    assert(terminal.apply_source_body_temperature_tick(-2.0f));
    assert(!terminal.state().alive);
    assert(terminal.state().death_reason==PlayerDeathReason::cold);
    assert(!terminal.apply_source_body_temperature_tick(-2.0f));
    return 0;
}
