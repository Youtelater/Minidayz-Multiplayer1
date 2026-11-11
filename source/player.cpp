#include "minidayz/player.hpp"

#include <algorithm>
#include <cmath>

namespace minidayz {
namespace {

float finite_or_zero(float value) noexcept {
    return std::isfinite(value) ? value : 0.0f;
}

std::uint32_t facing_offset(PlayerFacing facing) noexcept {
    switch (facing) {
    case PlayerFacing::down: return 0;
    case PlayerFacing::up: return 1;
    case PlayerFacing::right: return 2;
    case PlayerFacing::left: return 3;
    }
    return 0;
}

} // namespace

PlayerSystem::PlayerSystem(const PlayerState& initial) noexcept : state_(initial) {
    state_.x = finite_or_zero(state_.x);
    state_.y = finite_or_zero(state_.y);
    state_.health = std::max(0.0f, finite_or_zero(state_.health));
    state_.hunger = std::max(0.0f, finite_or_zero(state_.hunger));
    state_.thirst = std::max(0.0f, finite_or_zero(state_.thirst));
    state_.temperature = finite_or_zero(state_.temperature);
    state_.bleeding = std::max(0.0f, finite_or_zero(state_.bleeding));
    state_.infection = std::max(0.0f, finite_or_zero(state_.infection));
    state_.animation_elapsed = std::max(0.0f, finite_or_zero(state_.animation_elapsed));
    update_life_state();
}

float PlayerSystem::clamp(float value, float low, float high) noexcept {
    if (!std::isfinite(value)) return low;
    return std::max(low, std::min(value, high));
}

void PlayerSystem::update_life_state() noexcept {
    if (!state_.alive || state_.health <= 0.0f) {
        state_.health = 0.0f;
        state_.alive = false;
        state_.animation = PlayerAnimation::dead;
        state_.animation_frame = 0;
        state_.animation_elapsed = 0.0f;
        if (state_.death_reason == PlayerDeathReason::none)
            state_.death_reason = PlayerDeathReason::other;
    }
}

std::uint16_t PlayerSystem::source_animation_index() const noexcept {
    if (!state_.alive || state_.animation == PlayerAnimation::dead) return 0;
    const std::uint16_t direction = static_cast<std::uint16_t>(facing_offset(state_.facing));
    if (state_.animation == PlayerAnimation::idle) return direction;
    // The extracted player actor has idle_* and run_* animations but no
    // walk_* animation. Both movement speeds therefore use the source run pose.
    return static_cast<std::uint16_t>(4 + direction);
}

void PlayerSystem::advance_animation(float delta_seconds) noexcept {
    if (!state_.alive || state_.animation == PlayerAnimation::dead ||
        state_.animation == PlayerAnimation::idle) {
        state_.animation_frame = 0;
        state_.animation_elapsed = 0.0f;
        return;
    }

    // animations.csv records speed=1 and four looping frames for run_*.
    constexpr float kSourceRunFramesPerSecond = 1.0f;
    constexpr std::uint32_t kSourceRunFrameCount = 4;
    state_.animation_elapsed += std::max(0.0f, delta_seconds);
    const auto elapsed_frames = static_cast<std::uint32_t>(
        state_.animation_elapsed * kSourceRunFramesPerSecond);
    state_.animation_frame = elapsed_frames % kSourceRunFrameCount;
}

PlayerMoveResult PlayerSystem::update_movement(
    const PlayerInput& input,
    float delta_seconds,
    const PlayerRules& rules,
    PlayerCollisionQuery is_blocked,
    void* collision_context) noexcept {
    PlayerMoveResult result{};
    if (!state_.alive || !std::isfinite(delta_seconds) || delta_seconds <= 0.0f)
        return result;

    const float dt = clamp(delta_seconds, 0.0f, 0.05f);
    float ix = clamp(finite_or_zero(input.move_x), -1.0f, 1.0f);
    float iy = clamp(finite_or_zero(input.move_y), -1.0f, 1.0f);
    const float magnitude = std::sqrt(ix * ix + iy * iy);
    if (magnitude > 1.0f) {
        ix /= magnitude;
        iy /= magnitude;
    }

    const bool moving = std::fabs(ix) > 0.001f || std::fabs(iy) > 0.001f;
    if (!moving) {
        if (state_.animation != PlayerAnimation::idle) {
            state_.animation_elapsed = 0.0f;
            state_.animation_frame = 0;
        }
        state_.animation = PlayerAnimation::idle;
        return result;
    }

    if (std::fabs(ix) > std::fabs(iy))
        state_.facing = ix < 0.0f ? PlayerFacing::left : PlayerFacing::right;
    else
        state_.facing = iy < 0.0f ? PlayerFacing::up : PlayerFacing::down;

    const PlayerAnimation next_animation =
        input.run ? PlayerAnimation::run : PlayerAnimation::walk;
    if (state_.animation != next_animation) {
        state_.animation_elapsed = 0.0f;
        state_.animation_frame = 0;
    }
    state_.animation = next_animation;

    const float speed = input.run ? rules.run_speed : rules.walk_speed;
    const float wanted_x = ix * std::max(0.0f, finite_or_zero(speed)) * dt;
    const float wanted_y = iy * std::max(0.0f, finite_or_zero(speed)) * dt;
    const float half_w = std::max(0.0f, finite_or_zero(rules.collision_half_width));
    const float half_h = std::max(0.0f, finite_or_zero(rules.collision_half_height));

    const auto blocked = [&](float x, float y) noexcept {
        if (!is_blocked) return false;
        return is_blocked(collision_context, x - half_w, y - half_h) ||
               is_blocked(collision_context, x + half_w, y - half_h) ||
               is_blocked(collision_context, x - half_w, y + half_h) ||
               is_blocked(collision_context, x + half_w, y + half_h);
    };

    // Resolve each axis independently so the player can slide along walls.
    const float next_x = state_.x + wanted_x;
    if (!blocked(next_x, state_.y)) {
        state_.x = next_x;
        result.dx = wanted_x;
    } else {
        result.blocked_x = true;
    }

    const float next_y = state_.y + wanted_y;
    if (!blocked(state_.x, next_y)) {
        state_.y = next_y;
        result.dy = wanted_y;
    } else {
        result.blocked_y = true;
    }

    if (result.dx == 0.0f && result.dy == 0.0f) {
        state_.animation = PlayerAnimation::idle;
        state_.animation_frame = 0;
        state_.animation_elapsed = 0.0f;
    } else {
        advance_animation(dt);
    }
    return result;
}

PlayerSurvivalResult PlayerSystem::update_survival(
    float delta_seconds, const PlayerRules& rules) noexcept {
    PlayerSurvivalResult result{};
    if (!state_.alive || !std::isfinite(delta_seconds) || delta_seconds <= 0.0f)
        return result;

    const float dt = clamp(delta_seconds, 0.0f, 0.25f);
    const float old_health = state_.health;
    const float old_hunger = state_.hunger;
    const float old_thirst = state_.thirst;

    const float hunger_after_drain = std::max(0.0f, state_.hunger -
        std::max(0.0f, finite_or_zero(rules.hunger_drain_per_second)) * dt);
    const float thirst_after_drain = std::max(0.0f, state_.thirst -
        std::max(0.0f, finite_or_zero(rules.thirst_drain_per_second)) * dt);
    const float hunger_cap = std::max(0.0f, finite_or_zero(rules.max_hunger));
    const float thirst_cap = std::max(0.0f, finite_or_zero(rules.max_thirst));
    // A zero cap means "not configured", not an instruction to erase the stat.
    state_.hunger = hunger_cap > 0.0f
        ? std::min(hunger_after_drain, hunger_cap) : hunger_after_drain;
    state_.thirst = thirst_cap > 0.0f
        ? std::min(thirst_after_drain, thirst_cap) : thirst_after_drain;

    if (state_.hunger <= 0.0f) {
        const float damage =
            std::max(0.0f, finite_or_zero(rules.starvation_damage_per_second)) * dt;
        apply_damage(damage, PlayerDeathReason::hunger);
    }
    if (state_.thirst <= 0.0f) {
        const float damage =
            std::max(0.0f, finite_or_zero(rules.dehydration_damage_per_second)) * dt;
        apply_damage(damage, PlayerDeathReason::thirst);
    }
    if (state_.bleeding > 0.0f)
        apply_damage(state_.bleeding *
            std::max(0.0f, finite_or_zero(rules.bleeding_damage_per_severity_second)) * dt);
    if (state_.infection > 0.0f)
        apply_damage(state_.infection *
            std::max(0.0f, finite_or_zero(rules.infection_damage_per_severity_second)) * dt);

    if (state_.alive && rules.regeneration_per_second > 0.0f &&
        rules.max_health > 0.0f) {
        state_.health = clamp(state_.health +
            finite_or_zero(rules.regeneration_per_second) * dt,
            0.0f, rules.max_health);
    }

    if (rules.temperature_change_per_second != 0.0f) {
        const float low = std::min(finite_or_zero(rules.min_temperature),
                                   finite_or_zero(rules.max_temperature));
        const float high = std::max(finite_or_zero(rules.min_temperature),
                                    finite_or_zero(rules.max_temperature));
        state_.temperature = clamp(state_.temperature +
            finite_or_zero(rules.temperature_change_per_second) * dt, low, high);
    }

    update_life_state();
    result.health_lost = std::max(0.0f, old_health - state_.health);
    result.hunger_lost = std::max(0.0f, old_hunger - state_.hunger);
    result.thirst_lost = std::max(0.0f, old_thirst - state_.thirst);
    result.died = !state_.alive;
    return result;
}

void PlayerSystem::apply_damage(float amount) noexcept {
    apply_damage(amount, PlayerDeathReason::other);
}

void PlayerSystem::apply_damage(float amount, PlayerDeathReason cause) noexcept {
    if (!state_.alive || !std::isfinite(amount) || amount <= 0.0f) return;
    state_.health = std::max(0.0f, state_.health - amount);
    if (state_.health <= 0.0f && cause != PlayerDeathReason::none)
        state_.death_reason = cause;
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
    if (!state_.alive || !std::isfinite(amount) || amount <= 0.0f ||
        !std::isfinite(max_health) || max_health <= 0.0f) return;
    state_.health = clamp(state_.health + amount, 0.0f, max_health);
}

void PlayerSystem::adjust_hunger(float amount, float max_hunger) noexcept {
    if (!state_.alive || !std::isfinite(amount) || !std::isfinite(max_hunger) ||
        max_hunger <= 0.0f) return;
    state_.hunger = clamp(state_.hunger + amount, 0.0f, max_hunger);
}

void PlayerSystem::adjust_thirst(float amount, float max_thirst) noexcept {
    if (!state_.alive || !std::isfinite(amount) || !std::isfinite(max_thirst) ||
        max_thirst <= 0.0f) return;
    state_.thirst = clamp(state_.thirst + amount, 0.0f, max_thirst);
}

void PlayerSystem::adjust_temperature(float amount, float min_temperature,
                                      float max_temperature) noexcept {
    if (!std::isfinite(amount) || !std::isfinite(min_temperature) ||
        !std::isfinite(max_temperature)) return;
    const float low = std::min(min_temperature, max_temperature);
    const float high = std::max(min_temperature, max_temperature);
    state_.temperature = clamp(state_.temperature + amount, low, high);
}

void PlayerSystem::set_bleeding(float severity, float max_severity) noexcept {
    if (!std::isfinite(severity) || !std::isfinite(max_severity)) return;
    state_.bleeding = clamp(severity, 0.0f, std::max(0.0f, max_severity));
}

void PlayerSystem::set_infection(float severity, float max_severity) noexcept {
    if (!std::isfinite(severity) || !std::isfinite(max_severity)) return;
    state_.infection = clamp(severity, 0.0f, std::max(0.0f, max_severity));
}

} // namespace minidayz
