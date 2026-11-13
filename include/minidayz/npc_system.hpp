#pragma once

#include "minidayz/world.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidayz {

struct NpcActor {
    // Construct's Spawn_NPC creates a controller/eyes sprite (256) and a
    // skin-specific body sprite. The source spawn marker (216) is not rendered.
    std::uint16_t source_object{206};
    std::uint16_t eyes_object{256};
    std::uint16_t spawn_source_object{216};
    std::uint8_t skin_id{2};
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

// Native actors are created from the original source Spawn_NPC semantics.
// Skin IDs 1..6 select the same body objects as the Construct function;
// skin ID 0 requests the function's random 1..6 selection.
class NpcSystem {
public:
    bool initialize(const WorldSystem& world) noexcept;
    bool spawn_at(float x, float y, float spawn_offset, std::uint8_t skin_id,
                  std::uint64_t source_uid = 0) noexcept;
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
