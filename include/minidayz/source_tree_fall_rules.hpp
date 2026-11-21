#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::world_source {

// Source: root data.deobfuscated.js, Game_events > Bullets parameters >
// Hitting_obstacles. These records describe ONLY the original <=0
// tree-hp fall branch; they do not authorize a hit or define initial HP.
// Spawn_drop is called once for every item below; these are source item IDs,
// not Construct sprite object IDs. Event guard and RNG order are handled
// by the caller when implementing full collision/damage behavior.
struct TreeFallRule {
    std::uint16_t object_id;
    std::uint64_t event_uid;
    std::uint8_t count;
    std::array<std::uint16_t,4> drop_item_ids;
    std::uint16_t fall_animation;
    std::uint32_t fall_frame;
};
inline constexpr std::array<TreeFallRule,6> kTreeFallRules{{
    {184,8821937621821993ULL,2,{{50,50,0,0}},0,1},
    {207,2738244684146745ULL,3,{{38,38,50,0}},0,1},
    {209,7838407830495979ULL,4,{{38,38,50,37}},0,1},
    {576,4972733237327446ULL,2,{{50,50,0,0}},0,1},
    {577,9345628732648648ULL,3,{{50,50,38,0}},0,1},
    {581,4043087112077636ULL,4,{{50,50,38,38}},0,1}
}};
inline const TreeFallRule* tree_fall_rule(std::uint16_t object_id) noexcept {
    for (const auto& rule : kTreeFallRules)
        if (rule.object_id == object_id) return &rule;
    return nullptr;
}

// Exact original Game_events > Hitting_obstacles ancestor branches (root
// data.deobfuscated.js, verified by trace-tree-hit-source CI):
// sprite 345 tests the collision/power guard, then command 87 subtracts
// one from tree instance variable 0, BEFORE the fall child compares <= 0.
// These UIDs identify the hit event, not permission for arbitrary hits.
struct TreeHitRule {
    std::uint16_t object_id;
    std::uint64_t hit_event_uid;
    std::uint64_t fall_event_uid;
};
inline constexpr std::array<TreeHitRule,6> kTreeHitRules{{
    {184,7227858680569601ULL,8821937621821993ULL},
    {207,8754965976446553ULL,2738244684146745ULL},
    {209,5386206472407495ULL,7838407830495979ULL},
    {576,9193333425135665ULL,4972733237327446ULL},
    {577,4761103342332918ULL,9345628732648648ULL},
    {581,3531921662641626ULL,4043087112077636ULL}
}};
inline const TreeHitRule* tree_hit_rule(std::uint16_t object_id) noexcept {
    for (const auto& rule : kTreeHitRules)
        if (rule.object_id == object_id) return &rule;
    return nullptr;
}

// Original falling action for all six species also selects a sound
// from treefall_02/treefall_03, sets frame 1, invokes Spawn_drop
// at (Tree.X+random(-30,30), Tree.Y+40+random(10)) for each item,
// and sets the tutorTreeChopped source variable. Hit branches play a
// random chop/chop2/chop3/chop4 sound and subtract 1 from tree variable 0.
// Sound, random calls, collision, tutorial progression, initial tree HP
// and persistent world state must all be integrated in source event order.
} // namespace minidayz::world_source
