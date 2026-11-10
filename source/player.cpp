#include "minidayz/player.hpp"

#include <algorithm>
#include <cmath>

namespace minidayz {

PlayerSystem::PlayerSystem(const PlayerState& initial) noexcept : state_(initial) {
    update_life_state();
}

float PlayerSystem::clamp(float value, float low, float high) noexcept {
    return std::max(low, std::min(value, high));
}

void PlayerSystem::update_life_state() noexcept {
    if (state_.health <= 0.0f) {
        state_.health = 0.0f;
        state_.alive = false;
        state_.animation = PlayerAnimation::dead;
        if (state_.death_reason == PlayerDeathReason::none)
            state_.death_reason = PlayerDeathReason::other;
    }
}

PlayerMoveResult PlayerSystem::update_movement(
    const PlayerInput& input,
    float delta_seconds,
    const PlayerRules& rules,
    PlayerCollisionQuery is_blocked,
    void* collision_context) noexcept {
    PlayerMoveResult result{};
    if (!state_.alive || delta_seconds <= 0.0f) return result;

    const float dt = clamp(delta_seconds, 0.0f, 0.05f);
    float ix = clamp(input.move_x, -1.0f, 1.0f);
    float iy = clamp(input.move_y, -1.0f, 1.0f);
    const float magnitude = std::sqrt(ix * ix + iy * iy);
    if (magnitude > 1.0f) {
        ix /= magnitude;
        iy /= magnitude;
    }

    const bool moving = std::fabs(ix) > 0.001f || std::fabs(iy) > 0.001f;
    if (!moving) {
        state_.animation = PlayerAnimation::idle;
        return result;
    }

    if (std::fabs(ix) > std::fabs(iy))
        state_.facing = ix < 0.0f ? PlayerFacing::left : PlayerFacing::right;
    else
        state_.facing = iy < 0.0f ? PlayerFacing::up : PlayerFacing::down;

    const float speed = input.run ? rules.run_speed : rules.walk_speed;
    const float wanted_x = ix * std::max(0.0f, speed) * dt;
    const float wanted_y = iy * std::max(0.0f, speed) * dt;

    // Resolve each axis independently to preserve wall sliding.
    const float next_x = state_.x + wanted_x;
    if (!is_blocked || !is_blocked(collision_context, next_x, state_.y)) {
        state_.x = next_x;
        result.dx = wanted_x;
    } else {
        result.blocked_x = true;
    }

    const float next_y = state_.y + wanted_y;
    if (!is_blocked || !is_blocked(collision_context, state_.x, next_y)) {
        state_.y = next_y;
        result.dy = wanted_y;
    } else {
        result.blocked_y = true;
    }

    state_.animation = input.run ? PlayerAnimation::run : PlayerAnimation::walk;
    return result;
}

PlayerSurvivalResult PlayerSystem::update_survival(
    float delta_seconds, const PlayerRules& rules) noexcept {
    PlayerSurvivalResult result{};
    if (!state_.alive || delta_seconds <= 0.0f) return result;

    const float dt = clamp(delta_seconds, 0.0f, 0.25f);
    const float old_health = state_.health;
    const float old_hunger = state_.hunger;
    const float old_thirst = state_.thirst;

    state_.hunger = clamp(state_.hunger -
        std::max(0.0f, rules.hunger_drain_per_second) * dt,
        0.0f, std::max(0.0f, rules.max_hunger));
    state_.thirst = clamp(state_.thirst -
        std::max(0.0f, rules.thirst_drain_per_second) * dt,
        0.0f, std::max(0.0f, rules.max_thirst));

    // Damage from depleted food/water reserves is applied only when the
    // caller supplies source-audited damage rates.
    if (state_.hunger <= 0.0f) {
        const float damage =
            std::max(0.0f, rules.starvation_damage_per_second) * dt;
        const float health_before = state_.health;
        apply_damage(damage);
        if (health_before > 0.0f && !state_.alive)
            state_.death_reason = PlayerDeathReason::hunger;
    }
    if (state_.thirst <= 0.0f) {
        const float damage =
            std::max(0.0f, rules.dehydration_damage_per_second) * dt;
        const float health_before = state_.health;
        apply_damage(damage);
        if (health_before > 0.0f && !state_.alive)
            state_.death_reason = PlayerDeathReason::thirst;
    }

    if (state_.bleeding > 0.0f)
        apply_damage(state_.bleeding *
            std::max(0.0f, rules.bleeding_damage_per_severity_second) * dt);
    if (state_.infection > 0.0f)
        apply_damage(state_.infection *
            std::max(0.0f, rules.infection_damage_per_severity_second) * dt);

    if (state_.alive && rules.regeneration_per_second > 0.0f &&
        rules.max_health > 0.0f) {
        state_.health = clamp(state_.health +
            rules.regeneration_per_second * dt, 0.0f, rules.max_health);
    }

    if (rules.temperature_change_per_second != 0.0f) {
        const float low = std::min(rules.min_temperature, rules.max_temperature);
        const float high = std::max(rules.min_temperature, rules.max_temperature);
        state_.temperature = clamp(state_.temperature +
            rules.temperature_change_per_second * dt, low, high);
    }

    update_life_state();
    result.health_lost = std::max(0.0f, old_health - state_.health);
    result.hunger_lost = std::max(0.0f, old_hunger - state_.hunger);
    result.thirst_lost = std::max(0.0f, old_thirst - state_.thirst);
    result.died = !state_.alive;
    return result;
}

void PlayerSystem::apply_damage(float amount) noexcept {
    if (!state_.alive || amount <= 0.0f) return;
    state_.health = std::max(0.0f, state_.health - amount);
    update_life_state();
}

void PlayerSystem::kill(PlayerDeathReason reason) noexcept {
    if (!state_.alive) return;
    state_.death_reason = reason == PlayerDeathReason::none
        ? PlayerDeathReason::other : reason;
    state_.health = 0.0f;
    update_life_state();
}

void PlayerSystem::heal(float amount, float max_health) noexcept {
    if (!state_.alive || amount <= 0.0f || max_health <= 0.0f) return;
    state_.health = clamp(state_.health + amount, 0.0f, max_health);
}

void PlayerSystem::adjust_hunger(float amount, float max_hunger) noexcept {
    if (max_hunger <= 0.0f) return;
    state_.hunger = clamp(state_.hunger + amount, 0.0f, max_hunger);
}

void PlayerSystem::adjust_thirst(float amount, float max_thirst) noexcept {
    if (max_thirst <= 0.0f) return;
    state_.thirst = clamp(state_.thirst + amount, 0.0f, max_thirst);
}

void PlayerSystem::adjust_temperature(float amount, float min_temperature,
                                      float max_temperature) noexcept {
    const float low = std::min(min_temperature, max_temperature);
    const float high = std::max(min_temperature, max_temperature);
    state_.temperature = clamp(state_.temperature + amount, low, high);
}

void PlayerSystem::set_bleeding(float severity, float max_severity) noexcept {
    state_.bleeding = clamp(severity, 0.0f, std::max(0.0f, max_severity));
}

void PlayerSystem::set_infection(float severity, float max_severity) noexcept {
    state_.infection = clamp(severity, 0.0f, std::max(0.0f, max_severity));
}

} // namespace minidayz
