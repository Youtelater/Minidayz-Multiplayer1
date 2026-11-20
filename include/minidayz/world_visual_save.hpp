#pragma once
#include "minidayz/world.hpp"
#include <cstdint>

namespace minidayz::persistence {

// A small separately versioned world-object state journal. The existing
// player/inventory save ABI remains unchanged; validate world seed and level
// before applying even one saved tree, house or door frame.
bool write_world_visual_state(const WorldSystem& world, std::uint8_t level,
                              const char* target, const char* temporary) noexcept;
bool read_world_visual_state(WorldSystem& world, std::uint8_t level,
                             const char* path) noexcept;

} // namespace minidayz::persistence
