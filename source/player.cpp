#include "minidayz/player.hpp"

#include <algorithm>
#include <cmath>

namespace minidayz {

float PlayerSystem::clamp(float value, float low, float high) noexcept {
    return std::max(low, std::min(value, high));
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

    if (std::fabs(ix) > 0.001f || std::fabs(iy) > 0.001f) {
        if (std::fabs(ix) > std::fabs(iy))
            state_.facing = ix < 0.0f ? PlayerFacing::left : PlayerFacing::right;
        else
            state_.facing = iy < 0.0f ? PlayerFacing::up : PlayerFacing::down;
    }

    const float speed = input.run ? rules.run_speed : rules.walk_speed;
    const float wanted_x = ix * speed * dt;
    const float wanted_y = iy * speed * dt;

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
    return result;
}

void PlayerSystem::apply_damage(float amount) noexcept {
    if (!state_.alive || amount <= 0.0f) return;
    state_.health = std::max(0.0f, state_.health - amount);
    if (state_.health <= 0.0f) state_.alive = false;
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

void PlayerSystem::adjust_temperature(float amount) noexcept {
    state_.temperature += amount;
}

void PlayerSystem::set_bleeding(float severity) noexcept {
    state_.bleeding = std::max(0.0f, severity);
}

void PlayerSystem::set_infection(float severity) noexcept {
    state_.infection = std::max(0.0f, severity);
}

} // namespace minidayz
