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

    
    // Movement API coverage: block X while allowing Y-axis wall sliding.
    PlayerState moving_state{};
    moving_state.health = 100.0f;
    PlayerSystem moving(moving_state);
    PlayerRules movement_rules{};
    movement_rules.walk_speed = 100.0f;
    PlayerInput diagonal{};
    diagonal.move_x = 1.0f;
    diagonal.move_y = 1.0f;
    const auto blocked_x = +[](void*, float x, float) noexcept { return x > 0.0f; };
    const auto move_result = moving.update_movement(diagonal, 0.05f,
        movement_rules, blocked_x, nullptr);
    assert(move_result.blocked_x && !move_result.blocked_y);
    assert(move_result.dx == 0.0f && move_result.dy > 0.0f);
    assert(moving.state().x == 0.0f && moving.state().y > 0.0f);
    assert(moving.state().animation == PlayerAnimation::walk);

    // Injected survival rates are applied over a bounded frame. These are
    // API tests, not claims about the original game's tuned gameplay values.
    PlayerState survival_state{};
    survival_state.health = 50.0f;
    survival_state.hunger = 10.0f;
    survival_state.thirst = 10.0f;
    PlayerSystem survival(survival_state);
    PlayerRules survival_rules{};
    survival_rules.max_health = 100.0f;
    survival_rules.max_hunger = 100.0f;
    survival_rules.max_thirst = 100.0f;
    survival_rules.hunger_drain_per_second = 20.0f;
    survival_rules.thirst_drain_per_second = 20.0f;
    survival_rules.regeneration_per_second = 4.0f;
    survival_rules.temperature_change_per_second = 2.0f;
    survival_rules.min_temperature = -10.0f;
    survival_rules.max_temperature = 10.0f;
    const auto survival_result = survival.update_survival(0.25f, survival_rules);
    assert(!survival_result.died);
    assert(survival.state().health == 51.0f);
    assert(survival.state().hunger == 5.0f);
    assert(survival.state().thirst == 5.0f);
    assert(survival.state().temperature == 0.5f);
    assert(survival_result.hunger_lost == 5.0f);
    assert(survival_result.thirst_lost == 5.0f);

    // Stat setters clamp to the limits supplied by the caller.
    survival.adjust_hunger(200.0f, 100.0f);
    survival.adjust_thirst(-200.0f, 100.0f);
    survival.adjust_temperature(100.0f, -5.0f, 5.0f);
    survival.set_bleeding(3.0f, 2.0f);
    survival.set_infection(-1.0f, 4.0f);
    assert(survival.state().hunger == 100.0f);
    assert(survival.state().thirst == 0.0f);
    assert(survival.state().temperature == 5.0f);
    assert(survival.state().bleeding == 2.0f);
    assert(survival.state().infection == 0.0f);

    return 0;
}
