#pragma once

#include "minidayz/world.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidayz {

struct NpcActor {
    // Construct separates the zed_resp_point marker (216) from the visible
    // zombie body and its independently animated eyes overlay.
    std::uint16_t source_object{206};
    std::uint16_t eyes_object{256};
    std::uint16_t spawn_source_object{216};
    float x{}, y{};
    float home_x{}, home_y{};
    float health{100.0f};
    float animation_elapsed{};
    std::uint32_t animation_frame{};
    std::uint32_t random_state{};
    std::uint8_t animation_index{};
    bool alive{true};
};

using NpcCollisionQuery = bool (*)(void*, float, float) noexcept;

// Native actors are created from original source Zed spawn-point placements.
// The source marker is not rendered as the actor: body/eyes objects use the
// original Construct sprite IDs.
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
