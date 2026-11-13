#include "minidayz/npc_system.hpp"

#include <algorithm>
#include <cmath>

namespace minidayz {
namespace {
float clamp_delta(float value) noexcept {
    if (!std::isfinite(value) || value <= 0.0f) return 0.0f;
    return std::min(value, 0.05f);
}
std::uint32_t next_random(std::uint32_t& state) noexcept {
    if (state == 0) state = 0x9e3779b9u;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
}

bool NpcSystem::initialize(const WorldSystem& world) noexcept {
    reset();
    try {
        // Object 216 is the original zed_resp_point marker, not a zombie
        // sprite. The source Spawn_NPC function creates a body plus a separate
        // eyes object; use the normal skin pair as the native fallback until
        // the source callsite's skin-selection branch is evaluated.
        for (std::size_t i = 0; i < world.placement_count(); ++i) {
            const auto* placement = world.placement(i);
            if (!placement || placement->object != 216) continue;
            NpcActor actor{};
            actor.source_object = 206; // zed_normal_skin1
            actor.eyes_object = 256;    // zed_normal_eyes
            actor.spawn_source_object = placement->object;
            actor.x = actor.home_x = placement->x;
            actor.y = actor.home_y = placement->y;
            actor.random_state = static_cast<std::uint32_t>(
                placement->source_uid ^ (placement->source_uid >> 32) ^
                static_cast<std::uint64_t>(i + 1));
            actors_.push_back(actor);
        }
    } catch (...) {
        reset();
        return false;
    }
    return true;
}

void NpcSystem::reset() noexcept {
    actors_.clear();
}

void NpcSystem::update(float delta_seconds, float player_x, float player_y,
                       NpcCollisionQuery blocked, void* collision_context) noexcept {
    const float dt = clamp_delta(delta_seconds);
    if (dt == 0.0f || !std::isfinite(player_x) || !std::isfinite(player_y)) return;
    constexpr float kNoticeRadius = 190.0f;
    constexpr float kMoveSpeed = 25.0f;
    constexpr float kWanderRadius = 30.0f;
    constexpr float kCollisionRadius = 5.0f;
    for (auto& actor : actors_) {
        if (!actor.alive) continue;
        const float dx = player_x - actor.x;
        const float dy = player_y - actor.y;
        const float distance_sq = dx * dx + dy * dy;
        float vx = 0.0f, vy = 0.0f;
        if (distance_sq > 1.0f && distance_sq < kNoticeRadius * kNoticeRadius) {
            const float inverse = 1.0f / std::sqrt(distance_sq);
            vx = dx * inverse;
            vy = dy * inverse;
        } else {
            const float hx = actor.home_x - actor.x;
            const float hy = actor.home_y - actor.y;
            const float home_sq = hx * hx + hy * hy;
            if (home_sq > kWanderRadius * kWanderRadius) {
                const float inverse = 1.0f / std::sqrt(home_sq);
                vx = hx * inverse;
                vy = hy * inverse;
            } else if ((next_random(actor.random_state) & 0x3ffu) < 8u) {
                const float angle = static_cast<float>(next_random(actor.random_state) & 0xffffu)
                    * (6.28318530718f / 65536.0f);
                vx = std::cos(angle);
                vy = std::sin(angle);
            }
        }
        const float step_x = vx * kMoveSpeed * dt;
        const float step_y = vy * kMoveSpeed * dt;
        const auto is_blocked = [&](float x, float y) noexcept {
            return blocked && blocked(collision_context, x, y);
        };
        if (!is_blocked(actor.x + step_x - kCollisionRadius, actor.y) &&
            !is_blocked(actor.x + step_x + kCollisionRadius, actor.y))
            actor.x += step_x;
        if (!is_blocked(actor.x, actor.y + step_y - kCollisionRadius) &&
            !is_blocked(actor.x, actor.y + step_y + kCollisionRadius))
            actor.y += step_y;

        if (std::fabs(step_x) + std::fabs(step_y) > 0.01f) {
            // Original body animation order: walk_down/up/left/right = 12..15.
            if (std::fabs(vx) > std::fabs(vy))
                actor.animation_index = vx < 0.0f ? 14u : 15u;
            else
                actor.animation_index = vy < 0.0f ? 13u : 12u;
            actor.animation_elapsed += dt;
            const std::uint32_t frame_count =
                (actor.animation_index == 12u || actor.animation_index == 13u) ? 3u : 4u;
            actor.animation_frame =
                static_cast<std::uint32_t>(actor.animation_elapsed * 8.0f) % frame_count;
        } else {
            // Preserve facing while selecting the corresponding idle animation.
            if (actor.animation_index == 12u) actor.animation_index = 0u;
            else if (actor.animation_index == 13u) actor.animation_index = 1u;
            else if (actor.animation_index == 15u) actor.animation_index = 2u;
            else if (actor.animation_index == 14u) actor.animation_index = 3u;
            actor.animation_elapsed = 0.0f;
            actor.animation_frame = 0;
        }
    }
}

const NpcActor* NpcSystem::actor(std::size_t index) const noexcept {
    return index < actors_.size() ? &actors_[index] : nullptr;
}

void NpcSystem::damage(std::size_t index, float amount) noexcept {
    if (index >= actors_.size() || !std::isfinite(amount) || amount <= 0.0f) return;
    auto& actor = actors_[index];
    actor.health = std::max(0.0f, actor.health - amount);
    if (actor.health <= 0.0f) actor.alive = false;
}

} // namespace minidayz
