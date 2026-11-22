#pragma once
#include "minidayz/world.hpp"
#include <cstdint>
#include <cmath>

namespace minidayz::world_source {
// Root-original Construct event 389950912936355:
// Player181 OnCollision with Sprite796, instance boolean1 != true,
// then Sprite796.SetFrame(1), Fade.StartFade, var1=true.
//
// This object is specifically the ORIGINAL bunker lootbox. Its four
// sprite animations each contain frames0/1. Do not treat painted house
// furnishings as separate containers or invent six-slot storage for them.
inline constexpr std::uint16_t kSourceBunkerLootbox=796;
inline constexpr std::uint16_t kSourceInteriorContainerLayer=18; // items_on_ground
inline constexpr std::uint32_t kSourceBunkerLootboxOpenedFrame=1;
inline constexpr std::uint64_t kSourceBunkerLootboxOpenEvent=389950912936355ULL;
inline bool source_bunker_lootbox_closed(const WorldPlacement& box) noexcept {
    return box.object==kSourceBunkerLootbox &&
        box.layer==kSourceInteriorContainerLayer &&
        box.runtime_uid!=0 &&
        box.opacity>0.0f && std::isfinite(box.opacity) &&
        box.width>0.0f && box.height>0.0f &&
        box.source_animation<=3 && box.source_frame==0;
}
} // namespace minidayz::world_source
