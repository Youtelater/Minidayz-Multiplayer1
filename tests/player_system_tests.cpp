#include "minidayz/player.hpp"

#include <cassert>

using namespace minidayz;

int main() {
    PlayerState state{};
    state.health = 5.0f;
    state.hunger = 0.0f;
    state.thirst = 10.0f;
    PlayerSystem starvation(state);

    PlayerRules rules{};
    rules.max_health = 100.0f;
    rules.max_hunger = 100.0f;
    rules.max_thirst = 100.0f;
    rules.starvation_damage_per_second = 50.0f;

    const auto starvation_result = starvation.update_survival(0.1f, rules);
    assert(starvation_result.died);
    assert(!starvation.state().alive);
    assert(starvation.state().health == 0.0f);
    assert(starvation.state().death_reason == PlayerDeathReason::hunger);

    state.health = 5.0f;
    state.hunger = 10.0f;
    state.thirst = 0.0f;
    PlayerSystem dehydration(state);
    rules.starvation_damage_per_second = 0.0f;
    rules.dehydration_damage_per_second = 50.0f;

    const auto dehydration_result = dehydration.update_survival(0.1f, rules);
    assert(dehydration_result.died);
    assert(dehydration.state().death_reason == PlayerDeathReason::thirst);

    state.health = 5.0f;
    PlayerSystem ordinary_damage(state);
    ordinary_damage.apply_damage(10.0f);
    assert(!ordinary_damage.state().alive);
    assert(ordinary_damage.state().death_reason == PlayerDeathReason::other);

    state.health = 5.0f;
    PlayerSystem explicit_cause(state);
    explicit_cause.apply_damage(10.0f, PlayerDeathReason::cold);
    assert(!explicit_cause.state().alive);
    assert(explicit_cause.state().death_reason == PlayerDeathReason::cold);

    return 0;
}
