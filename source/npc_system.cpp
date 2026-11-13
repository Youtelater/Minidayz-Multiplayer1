#include "minidayz/npc_system.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

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
float random_symmetric(std::uint32_t& state, float radius) noexcept {
    const float unit = static_cast<float>(next_random(state) & 0x00ffffffu)
        / 16777215.0f;
    return (unit * 2.0f - 1.0f) * radius;
}
std::uint16_t body_for_skin(std::uint8_t skin_id) noexcept {
    switch (skin_id) {
    case 1: return 600;
    case 2: return 206;
    case 3: return 582;
    case 4: return 583;
    case 5: return 586;
    case 6: return 587;
    default: return 0;
    }
}
std::uint64_t stable_spawn_uid(const WorldPlacement& placement) noexcept {
    // Generated Construct placements currently have no persistent UID. Hash
    // their exact source coordinates/object so skin selection is stable for
    // the same generated world and does not collapse to one default skin.
    std::uint32_t x{}, y{};
    std::memcpy(&x, &placement.x, sizeof(x));
    std::memcpy(&y, &placement.y, sizeof(y));
    std::uint64_t value = (static_cast<std::uint64_t>(x) << 32) | y;
    value ^= static_cast<std::uint64_t>(placement.object) * 0x9e3779b97f4a7c15ULL;
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31;
    return value ? value : 0x6d696e696461797au;
}
}

bool NpcSystem::initialize(const WorldSystem& world) noexcept {
    reset();
    try {
        // Object 216 is the source zed_resp placement marker emitted by the
        // original generate_location event sheet. Spawn_NPC's Skin_id=0 path
        // selects one of its six original body variants.
        for (std::size_t i = 0; i < world.placement_count(); ++i) {
            const auto* placement = world.placement(i);
            if (!placement || placement->object != 216) continue;
            const std::uint64_t uid = placement->source_uid != 0
                ? placement->source_uid : stable_spawn_uid(*placement);
            if (!spawn_at(placement->x, placement->y, 0.0f, 0, uid)) {
                reset();
                return false;
            }
            actors_.back().spawn_source_object = placement->object;
        }
    } catch (...) {
        reset();
        return false;
    }
    return true;
}

bool NpcSystem::spawn_at(float x, float y, float spawn_offset,
                         std::uint8_t skin_id, std::uint64_t source_uid) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(spawn_offset) || spawn_offset < 0.0f ||
        spawn_offset > 10000.0f || skin_id > 6)
        return false;
    std::uint32_t state = static_cast<std::uint32_t>(
        source_uid ^ (source_uid >> 32) ^ 0x9e3779b9u);
    if (state == 0) state = 0x6d2b79f5u;
    if (skin_id == 0)
        skin_id = static_cast<std::uint8_t>(1u + next_random(state) % 6u);
    const std::uint16_t body = body_for_skin(skin_id);
    if (body == 0) return false;

    NpcActor actor{};
    actor.source_object = body;
    actor.eyes_object = 256;
    actor.spawn_source_object = 216;
    actor.skin_id = skin_id;
    actor.x = x + random_symmetric(state, spawn_offset);
    actor.y = y + random_symmetric(state, spawn_offset);
    actor.home_x = actor.x;
    actor.home_y = actor.y;
    actor.random_state = state;
    try {
        actors_.push_back(actor);
    } catch (...) {
        return false;
    }
    return true;
}

void NpcSystem::reset() noexcept { actors_.clear(); }

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
            vx = dx * inverse; vy = dy * inverse;
        } else {
            const float hx = actor.home_x - actor.x;
            const float hy = actor.home_y - actor.y;
            const float home_sq = hx * hx + hy * hy;
            if (home_sq > kWanderRadius * kWanderRadius) {
                const float inverse = 1.0f / std::sqrt(home_sq);
                vx = hx * inverse; vy = hy * inverse;
            } else if ((next_random(actor.random_state) & 0x3ffu) < 8u) {
                const float angle = static_cast<float>(next_random(actor.random_state) & 0xffffu)
                    * (6.28318530718f / 65536.0f);
                vx = std::cos(angle); vy = std::sin(angle);
            }
        }
        const float step_x = vx * kMoveSpeed * dt;
        const float step_y = vy * kMoveSpeed * dt;
        const auto is_blocked = [&](float x, float y) noexcept {
            return blocked && blocked(collision_context, x, y);
        };
        if (!is_blocked(actor.x + step_x - kCollisionRadius, actor.y) &&
            !is_blocked(actor.x + step_x + kCollisionRadius, actor.y)) actor.x += step_x;
        if (!is_blocked(actor.x, actor.y + step_y - kCollisionRadius) &&
            !is_blocked(actor.x, actor.y + step_y + kCollisionRadius)) actor.y += step_y;
        if (std::fabs(step_x) + std::fabs(step_y) > 0.01f) {
            if (std::fabs(vx) > std::fabs(vy))
                actor.animation_index = vx < 0.0f ? 14u : 15u;
            else actor.animation_index = vy < 0.0f ? 13u : 12u;
            actor.animation_elapsed += dt;
            const std::uint32_t frame_count =
                (actor.animation_index == 12u || actor.animation_index == 13u) ? 3u : 4u;
            actor.animation_frame =
                static_cast<std::uint32_t>(actor.animation_elapsed * 8.0f) % frame_count;
        } else {
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
