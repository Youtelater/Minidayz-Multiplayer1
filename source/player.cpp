#include "minidayz/player.hpp"
#include "minidayz/source_equipment_animation.hpp"
#include <cstring>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace minidayz {
namespace {

float finite_or_zero(float value) noexcept {
    return std::isfinite(value) ? value : 0.0f;
}

const char* source_facing_name(PlayerFacing facing) noexcept {
    switch (facing) {
    case PlayerFacing::down: return "down";
    case PlayerFacing::up: return "up";
    case PlayerFacing::right: return "right";
    case PlayerFacing::left: return "left";
    }
    return "down";
}

bool has_source_animation(std::uint16_t object, const char* name) noexcept {
    for (const auto& animation : stage1::kAnimations) {
        if (animation.object == object &&
            animation.frames > 0 && std::strcmp(animation.name, name) == 0)
            return true;
    }
    return false;
}

} // namespace

PlayerInput player_input_from_circle_pad(std::int16_t x,std::int16_t y) noexcept {
    constexpr float dead_zone=12.0f;
    constexpr float full_deflection=156.0f;
    const float dx=static_cast<float>(x);
    const float dy=static_cast<float>(y);
    const float radius=std::sqrt(dx*dx+dy*dy);
    if (radius<=dead_zone) return {};
    const float magnitude=(std::min(radius,full_deflection)-dead_zone) /
                          (full_deflection-dead_zone);
    const float scale=magnitude/radius;
    return {dx*scale,-dy*scale,false};
}

PlayerRules source_survival_rules(SurvivalDifficulty difficulty) noexcept {
    PlayerRules rules{};
    rules.max_health = 100.0f;
    rules.max_hunger = 100.0f;
    rules.max_thirst = 100.0f;
    float source_rate_per_five_seconds = 0.75f;
    switch (difficulty) {
    case SurvivalDifficulty::novice: source_rate_per_five_seconds = 0.5f; break;
    case SurvivalDifficulty::regular: source_rate_per_five_seconds = 0.75f; break;
    case SurvivalDifficulty::veteran: source_rate_per_five_seconds = 1.0f; break;
    }
    // Original Game_events sets Novice/Regular/Veteran_starving and _thirsty
    // to 0.5/0.75/1.0; Foodlimit and Waterlimit multiply by five seconds.
    rules.hunger_drain_per_second = source_rate_per_five_seconds / 5.0f;
    rules.thirst_drain_per_second = source_rate_per_five_seconds / 5.0f;
    return rules;
}

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

bool PlayerSystem::restore(const PlayerState& saved) noexcept {
    const auto finite = [](float value) noexcept { return std::isfinite(value); };
    if (!finite(saved.x) || !finite(saved.y) || !finite(saved.health) ||
        !finite(saved.hunger) || !finite(saved.thirst) ||
        !finite(saved.temperature) || !finite(saved.bleeding) ||
        !finite(saved.infection) || !finite(saved.animation_elapsed) ||
        saved.health < 0.0f || saved.hunger < 0.0f || saved.thirst < 0.0f ||
        saved.bleeding < 0.0f || saved.infection < 0.0f ||
        saved.animation_elapsed < 0.0f)
        return false;

    switch (saved.facing) {
    case PlayerFacing::down: case PlayerFacing::up:
    case PlayerFacing::left: case PlayerFacing::right: break;
    default: return false;
    }
    switch (saved.animation) {
    case PlayerAnimation::idle: case PlayerAnimation::walk:
    case PlayerAnimation::run: case PlayerAnimation::dead: break;
    default: return false;
    }
    switch (saved.death_reason) {
    case PlayerDeathReason::none: case PlayerDeathReason::hunger:
    case PlayerDeathReason::thirst: case PlayerDeathReason::cold:
    case PlayerDeathReason::other: break;
    default: return false;
    }
    if ((!saved.alive && saved.health != 0.0f) ||
        (saved.alive && (saved.health == 0.0f ||
                         saved.animation == PlayerAnimation::dead)))
        return false;

    state_ = saved;
    return true;
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

void PlayerSystem::set_source_skin(std::uint16_t object) noexcept {
    if (source_skin_object_ == object) return;
    // The original player skins must have all 8 directions, not just
    // idle_down and run_down. Otherwise a selected skin can silently
    // disappear when the survivor turns.
    for (const char* direction : {"down", "up", "right", "left"}) {
        char name[24]{};
        std::snprintf(name, sizeof(name), "idle_%s", direction);
        if (!has_source_animation(object, name)) return;
        std::snprintf(name, sizeof(name), "run_%s", direction);
        if (!has_source_animation(object, name)) return;
    }
    source_skin_object_=object;
    state_.animation_elapsed=0.0f;
    state_.animation_frame=0;
}

std::uint16_t PlayerSystem::source_animation_index() const noexcept {
    if (!state_.alive || state_.animation == PlayerAnimation::dead) return 0;
    // Original animation indices belong to EACH Construct sprite object.
    // Do not assume idle=0..3 and run=4..7 for every selectable survivor.
    // The original actor has no walk_*: both walking and running select
    // its original run_* animation.
    char wanted[24]{};
    std::snprintf(wanted, sizeof(wanted), "%s_%s",
        state_.animation == PlayerAnimation::idle ? "idle" : "run",
        source_facing_name(state_.facing));
    std::uint16_t index=0;
    for (const auto& animation : stage1::kAnimations) {
        if (animation.object != source_skin_object_) continue;
        if (animation.frames > 0 && std::strcmp(animation.name, wanted) == 0)
            return index;
        ++index;
    }
    return 0;
}

void PlayerSystem::advance_animation(float delta_seconds) noexcept {
    if (!state_.alive || state_.animation == PlayerAnimation::dead ||
        state_.animation == PlayerAnimation::idle) {
        state_.animation_frame = 0;
        state_.animation_elapsed = 0.0f;
        return;
    }

    // Every drawn layer needs the full elapsed time to resolve its own cycle.
    char name[24]{};
    std::snprintf(name,sizeof(name),"run_%s",source_facing_name(state_.facing));
    state_.animation_elapsed+=delta_seconds;
    state_.animation_frame=stage1::source_equipment_animation_frame(
        source_skin_object_,name,state_.animation_elapsed);
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

    // Honor measured gameplay time, including slow ~5-10 FPS frames.
    // The collision substeps below protect against tunneling.
    // Only cap emulator suspend/resume stalls (over 250 ms).
    const float dt = clamp(delta_seconds, 0.0f, 0.25f);
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

    const auto old_facing=state_.facing;
    if (std::fabs(ix) > std::fabs(iy))
        state_.facing = ix < 0.0f ? PlayerFacing::left : PlayerFacing::right;
    else
        state_.facing = iy < 0.0f ? PlayerFacing::up : PlayerFacing::down;
    if (state_.facing!=old_facing) {
        state_.animation_elapsed=0.0f;
        state_.animation_frame=0;
    }

    if (state_.animation == PlayerAnimation::idle) {
        state_.animation_elapsed = 0.0f;
        state_.animation_frame = 0;
    }
    // Walk and run use the same directional animation and retain its phase.
    state_.animation = input.run ? PlayerAnimation::run : PlayerAnimation::walk;

    const float speed = input.run ? rules.run_speed : rules.walk_speed;
    const float wanted_x = ix * std::max(0.0f, finite_or_zero(speed)) * dt;
    const float wanted_y = iy * std::max(0.0f, finite_or_zero(speed)) * dt;
    const float half_w = std::max(0.0f, finite_or_zero(rules.collision_half_width));
    const float half_h = std::max(0.0f, finite_or_zero(rules.collision_half_height));

    const auto blocked = [&](float x, float y) noexcept {
        if (!is_blocked) return false;
        // Construct position is at the source image HOTSPOT, not always
        // the centre of its collision rectangle (PlayerBase193 is origin1,1).
        x += finite_or_zero(rules.collision_offset_x);
        y += finite_or_zero(rules.collision_offset_y);
        return is_blocked(collision_context, x - half_w, y - half_h) ||
               is_blocked(collision_context, x + half_w, y - half_h) ||
               is_blocked(collision_context, x - half_w, y + half_h) ||
               is_blocked(collision_context, x + half_w, y + half_h);
    };

    // Sub-step collision probes: checking only the endpoint can tunnel
    // through a 30px water/forest tile or narrow building at high speed.
    // Keep X and Y independent to preserve the original wall-slide feel.
    // Sample at most one pixel apart so the 2x2 player cannot skip a
    // one-pixel solid band between collision probes on a slow frame.
    constexpr float kMaximumCollisionStep=1.0f;
    const int steps=is_blocked ? std::max(1,static_cast<int>(std::ceil(
        std::max(std::fabs(wanted_x),std::fabs(wanted_y))/
        kMaximumCollisionStep))) : 1;
    const float step_x=wanted_x/steps,step_y=wanted_y/steps;
    bool x_blocked=false,y_blocked=false;
    for(int step=0;step<steps;++step) {
        if(!x_blocked && step_x!=0.0f) {
            const float next=state_.x+step_x;
            if(blocked(next,state_.y)) {
                result.blocked_x=true;
                x_blocked=true;
            } else {
                state_.x=next;
                result.dx+=step_x;
            }
        }
        if(!y_blocked && step_y!=0.0f) {
            const float next=state_.y+step_y;
            if(blocked(state_.x,next)) {
                result.blocked_y=true;
                y_blocked=true;
            } else {
                state_.y=next;
                result.dy+=step_y;
            }
        }
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

void PlayerSystem::apply_damage(float amount,
                                const PlayerArmorProtection& protection) noexcept {
    const float armor = std::max(0.0f,
        finite_or_zero(protection.helmet) +
        finite_or_zero(protection.outerwear) +
        finite_or_zero(protection.vest) +
        finite_or_zero(protection.pants));
    apply_damage(std::max(0.0f, finite_or_zero(amount) - armor),
                 PlayerDeathReason::other);
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

void PlayerSystem::set_source_temperature(float source_value) noexcept {
    if (!state_.alive || !std::isfinite(source_value)) return;
    // Unlike preview meters, the Construct Day_cold - warmth assignment
    // accepts negative results. The persisted PlayerState uses float.
    state_.temperature = source_value;
}

bool PlayerSystem::apply_source_body_temperature_tick(
    float weather_exposure) noexcept {
    if (!state_.alive || !std::isfinite(weather_exposure) ||
        !std::isfinite(state_.temperature)) return false;
    // Original Construct Game_events Player stats -> States:
    // five-second condition UID 2665582317889917, action UID
    // 471091765829229: actor variable 16 += Player_temperature / 2.
    const float next = state_.temperature + weather_exposure / 2.0f;
    if (!std::isfinite(next)) return false;
    state_.temperature = next;
    if (state_.temperature < 0.0f) {
        // Original branch 8552060907536112 resets body cold to zero and
        // subtracts a quarter health via its nested source action.
        state_.temperature = 0.0f;
        apply_damage(0.25f, PlayerDeathReason::cold);
    }
    return true;
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
