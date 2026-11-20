#pragma once
#include "minidayz/world.hpp"
#include "minidayz/source_tree_fall_rules.hpp"
#include "minidayz/ground_loot.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace minidayz::world_source {

// Apply the ORIGINAL tree-fall branch only after its full Construct parent
// conditions have been resolved by combat/collision code. This executor
// deliberately does not guess weapon hit eligibility, HP or RNG. The caller
// must supply the exact, already evaluated Spawn_drop positions in source
// action order (Tree.X+random(-30,30), Tree.Y+40+random(10)).
inline bool apply_verified_tree_fall(
    WorldSystem& world, std::uint64_t runtime_uid,
    std::uint64_t source_event_uid,
    const std::array<WorldPoint,4>& source_drop_positions,
    items::GroundLootSystem& ground) noexcept {
    if(runtime_uid==0 || !world.generation_complete())return false;
    std::size_t index=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(p && p->runtime_uid==runtime_uid) {index=i;break;}
    }
    if(index==world.placement_count())return false;
    const auto* placed=world.placement(index);
    const auto* rule=tree_fall_rule(placed->object);
    if(!rule || source_event_uid!=rule->event_uid ||
       placed->source_animation!=rule->fall_animation ||
       placed->source_frame==rule->fall_frame)return false;
    if(ground.count()+rule->count > items::kMaximumGroundLootItems)
        return false;
    for(std::size_t i=0;i<rule->count;++i)
        if(!std::isfinite(source_drop_positions[i].x) ||
           !std::isfinite(source_drop_positions[i].y))
            return false;
    const auto original=*placed;
    // GroundLootSystem::add is validated by its own original-item catalog.
    // Save its previous state so a failed drop cannot leave a chopped
    // tree with missing resources.
    const auto ground_before=ground.snapshot();
    if(!world.set_placement_sprite_state(index,rule->fall_animation,
                                          rule->fall_frame,original.opacity))
        return false;
    for(std::size_t i=0;i<rule->count;++i) {
        const auto& where=source_drop_positions[i];
        if(!ground.add({rule->drop_item_ids[i],1,
                        items::kUseSourceItemCondition},where.x,where.y)) {
            (void)ground.restore(ground_before);
            (void)world.set_placement_sprite_state(
                index,original.source_animation,
                original.source_frame,original.opacity);
            return false;
        }
    }
    return true;
}

// Original automatic door event UIDs:
// 6061166023614368 sets door object 349 frame 1 (open);
// 243854400993751 sets frame 0 (close).
// Collision/shadow/sound companion actions are NOT implied by this helper;
// the caller must dispatch the full verified original event group.
inline bool apply_verified_door_frame(
    WorldSystem& world,std::uint64_t runtime_uid,
    std::uint64_t source_event_uid) noexcept {
    constexpr std::uint64_t kOpen=6061166023614368ULL;
    constexpr std::uint64_t kClose=243854400993751ULL;
    if(!world.generation_complete() || runtime_uid==0 ||
       (source_event_uid!=kOpen && source_event_uid!=kClose))
        return false;
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(!p || p->runtime_uid!=runtime_uid)continue;
        if(p->object!=349)return false;
        const std::uint32_t frame=source_event_uid==kOpen?1u:0u;
        if(p->source_frame==frame)return false;
        return world.set_placement_sprite_state(
            i,p->source_animation,frame,p->opacity);
    }
    return false;
}

} // namespace minidayz::world_source
