#pragma once

#include "minidayz/world.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidayz {

struct NpcActor {
    std::uint16_t source_object{216};
    float x{}, y{};
    float home_x{}, home_y{};
    float health{100.0f};
    float animation_elapsed{};
    std::uint32_t animation_frame{};
    std::uint32_t random_state{};
    bool alive{true};
};

using NpcCollisionQuery = bool (*)(void*, float, float) noexcept;

// Native actor lifecycle for source-created Zed objects. Actors are seeded
// only from placements emitted by the source location event runtime; no
// synthetic NPC locations are invented.
class NpcSystem {
public:
    bool initialize(const WorldSystem& world) noexcept;
    void reset() noexcept;
    void update(float delta_seconds, float player_x, float player_y,
                NpcCollisionQuery blocked = nullptr,
                void* collision_context = nullptr) noexcept;
    std::size_t count() const noexcept { return actors_.size(); }
    const NpcActor* actor(std::size_t index) const noexcept;
    void damage(std::size_t index, float amount) noexcept;

private:
    std::vector<NpcActor> actors_{};
};

} // namespace minidayz
