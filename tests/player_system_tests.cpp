#include "minidayz/player.hpp"

#include <cassert>
#include <cmath>
#include <limits>

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

    // The default player skin has idle down/up/right/left (0..3) and
    // run down/up/right/left (4..7); it has no dedicated death animation.
    PlayerState animation_state{};
    animation_state.health = 100.0f;
    PlayerSystem animated(animation_state);
    assert(animated.source_animation_index() == 0);
    PlayerInput up{};
    up.move_y = -1.0f;
    animated.update_movement(up, 0.05f, movement_rules);
    assert(animated.source_animation_index() == 5);
    assert(animated.source_animation_frame() == 0);
    for (int i = 0; i < 20; ++i)
        animated.update_movement(up, 0.05f, movement_rules);
    assert(animated.source_animation_frame() == 1);
    PlayerInput idle{};
    animated.update_movement(idle, 0.05f, movement_rules);
    assert(animated.source_animation_index() == 1);
    assert(animated.source_animation_frame() == 0);
    animated.kill(PlayerDeathReason::other);
    assert(animated.source_animation_index() == 0);
    assert(animated.source_animation_frame() == 0);

    // The configured player half-size participates in collision checks.
    PlayerState edge_state{};
    edge_state.x = 1.0f;
    edge_state.y = 10.0f;
    edge_state.health = 100.0f;
    PlayerSystem edge_player(edge_state);
    PlayerRules edge_rules{};
    edge_rules.walk_speed = 20.0f;
    edge_rules.collision_half_width = 2.0f;
    edge_rules.collision_half_height = 2.0f;
    const auto outside_world = +[](void*, float x, float y) noexcept {
        return x < 0.0f || y < 0.0f || x > 100.0f || y > 100.0f;
    };
    PlayerInput left{};
    left.move_x = -1.0f;
    const auto edge_move = edge_player.update_movement(
        left, 0.05f, edge_rules, outside_world, nullptr);
    assert(edge_move.blocked_x);
    assert(edge_player.state().x == 1.0f);

    // Non-finite input must not poison player coordinates or stats.
    PlayerInput invalid{};
    invalid.move_x = std::numeric_limits<float>::quiet_NaN();
    edge_player.update_movement(invalid, 0.05f, edge_rules);
    assert(edge_player.state().x == 1.0f);
    assert(std::isfinite(edge_player.state().x));
    assert(std::isfinite(edge_player.state().y));

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

    // Unconfigured zero caps must not erase stats while source values
    // are intentionally left unresolved.
    PlayerState unconfigured_state{};
    unconfigured_state.health = 50.0f;
    unconfigured_state.hunger = 37.0f;
    unconfigured_state.thirst = 62.0f;
    PlayerSystem unconfigured(unconfigured_state);
    const auto unchanged = unconfigured.update_survival(0.1f, PlayerRules{});
    assert(!unchanged.died);
    assert(unconfigured.state().hunger == 37.0f);
    assert(unconfigured.state().thirst == 62.0f);

    // Player save/load round-trips all persistent player state and rejects
    // corrupt numeric and enum values without modifying the live player.
    PlayerState persisted_state{};
    persisted_state.x = 12.5f;
    persisted_state.y = -8.0f;
    persisted_state.health = 73.0f;
    persisted_state.hunger = 44.0f;
    persisted_state.thirst = 29.0f;
    persisted_state.temperature = -2.0f;
    persisted_state.bleeding = 1.0f;
    persisted_state.infection = 0.25f;
    persisted_state.facing = PlayerFacing::left;
    persisted_state.animation = PlayerAnimation::run;
    persisted_state.animation_frame = 2;
    persisted_state.animation_elapsed = 2.5f;
    PlayerSystem save_source(persisted_state);
    const PlayerState save_payload = save_source.snapshot();
    PlayerSystem save_target(PlayerState{});
    assert(save_target.restore(save_payload));
    assert(save_target.snapshot().x == 12.5f);
    assert(save_target.snapshot().y == -8.0f);
    assert(save_target.snapshot().health == 73.0f);
    assert(save_target.snapshot().hunger == 44.0f);
    assert(save_target.snapshot().thirst == 29.0f);
    assert(save_target.snapshot().temperature == -2.0f);
    assert(save_target.snapshot().bleeding == 1.0f);
    assert(save_target.snapshot().infection == 0.25f);
    assert(save_target.snapshot().facing == PlayerFacing::left);
    assert(save_target.snapshot().animation == PlayerAnimation::run);
    assert(save_target.snapshot().animation_frame == 2);

    PlayerState corrupt_player = save_payload;
    corrupt_player.health = std::numeric_limits<float>::quiet_NaN();
    assert(!save_target.restore(corrupt_player));
    assert(save_target.state().health == 73.0f);
    corrupt_player = save_payload;
    corrupt_player.facing = static_cast<PlayerFacing>(255);
    assert(!save_target.restore(corrupt_player));
    corrupt_player = save_payload;
    corrupt_player.alive = false;
    assert(!save_target.restore(corrupt_player));

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
