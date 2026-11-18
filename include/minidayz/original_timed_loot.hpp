#pragma once
#include "minidayz/generated/original_timed_loot.hpp"
#include "minidayz/ground_loot.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// The original Game_events Global_respawn tree dispatches Trigger_spawn
// ONLY for LOOT_respawn markers whose source instance variable 0 is 15,
// whose distance to the player is >1000, on the Map layout, off bridges.
// The normal SEED_pubg=0 branch also excludes marker family 995 and has a
// source choose(1,2,3,4,5)!=1 gate. This is not a general loot generator.
struct OriginalTimedLootMarker {
    float x{}, y{};
    std::uint16_t category{};
    bool belongs_to_source_family_995{};
};
struct OriginalTimedLootContext {
    float player_x{}, player_y{};
    bool on_map_layout{};
    bool on_bridge{};
    bool check_loot_timer_fired{};
    std::uint8_t seed_pubg{}; // exact original source variable, 0 or 1
};

// Use one random draw for the original source eligibility choose(...) and
// another for the original weighted Spawn_drop choose(...).
// Other source conditions must not be bypassed; do not fabricate markers.
inline bool source_timed_loot_respawn(
    GroundLootSystem& ground,
    const OriginalTimedLootMarker& marker,
    const OriginalTimedLootContext& context,
    std::uint32_t eligibility_random,
    std::uint32_t item_random) noexcept {
    if (!context.check_loot_timer_fired || !context.on_map_layout ||
        context.on_bridge || context.seed_pubg > 1 ||
        marker.category != 15 ||
        !std::isfinite(marker.x) || !std::isfinite(marker.y) ||
        !std::isfinite(context.player_x) || !std::isfinite(context.player_y))
        return false;
    const double dx = static_cast<double>(marker.x) - context.player_x;
    const double dy = static_cast<double>(marker.y) - context.player_y;
    if (dx * dx + dy * dy <= 1000000.0) return false;

    // Source SEED_pubg=0 normal branch: source family 995 is excluded,
    // and choose(1,2,3,4,5) != 1 is required.
    if (context.seed_pubg == 0) {
        if (marker.belongs_to_source_family_995 ||
            (static_cast<std::uint64_t>(eligibility_random) * 5u >> 32) == 0)
            return false;
    }
    for (const auto& source : kOriginalTimedLootPools) {
        if (source.seed_pubg != context.seed_pubg ||
            source.count == 0 || source.first + source.count >
                kOriginalTimedLootChoiceCount) continue;
        const auto choice = static_cast<std::size_t>(
            (static_cast<std::uint64_t>(item_random) * source.count) >> 32);
        const auto id = kOriginalTimedLootChoices[source.first + choice];
        // Construct round() of marker X/Y (Math.round behavior).
        return ground.add({id, 1, kUseSourceItemCondition},
                          std::floor(marker.x + 0.5f),
                          std::floor(marker.y + 0.5f));
    }
    return false;
}

inline bool source_authored_map_loot_marker(
    std::uint64_t source_uid, std::uint16_t& category) noexcept {
    for (const auto& marker : kOriginalMapLootMarkers) {
        if (marker.source_uid == source_uid) {
            category = marker.category;
            return true;
        }
    }
    return false;
}
} // namespace minidayz::items
