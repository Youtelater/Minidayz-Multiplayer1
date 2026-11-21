#pragma once
#include "minidayz/world.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/source_world_event_effects.hpp"
#include "minidayz/generated/source_door_points.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace minidayz::world_source {

// Original Game_events > Doors_auto is an automatic interaction:
// Player_collision_base (181) overlaps door (349), then PlayerBase (193)
// compares its Y against Door.ImagePointY("in").
// Original open UID 6061166023614368: comparator >=; frame 0 -> 1.
// Original close UID 243854400993751: comparator <; frame 1 -> 0.
// Native overlap is a four-axis rotated-rectangle SAT approximation of Construct sprite
// collision, *not* a claim to reproduce its polygon/obstacle-base conditions.
// The source frame point and source audio family are exact.
enum class AutomaticDoorEffect : std::uint8_t {
    none, open_wood, open_metal, close_wood, close_metal
};

inline bool source_door_overlaps_player(
    const WorldPlacement& door, const SourceDoorPoint& frame,
    WorldPoint player) noexcept {
    // Use the same complete four-axis rectangle SAT as house overlap.
    // This rejects spurious door transitions near rotated corners.
    return source_player_overlaps_world_sprite(
        door,player,frame.origin_x,frame.origin_y);
}

inline float source_door_in_y(const WorldPlacement& door,
                             const SourceDoorPoint& frame) noexcept {
    const float a=door.angle*0.01745329251994329577f;
    const float dx=door.width*(frame.in_x-frame.origin_x)*
                   (door.scale_x<0.0f?-1.0f:1.0f);
    const float dy=door.height*(frame.in_y-frame.origin_y);
    return door.y+dx*std::sin(a)+dy*std::cos(a);
}

// Original Doors_auto chooses a transition from frame and ImagepointY("in").
// Pure selection allows the native gameplay scene to honor the source 0.01s
// Wait before changing the actual frame and playing its associated sound.
inline AutomaticDoorEffect source_auto_door_intended_for_placement(
    const WorldSystem& world, std::size_t index,
    WorldPoint player) noexcept {
    if(!world.generation_complete())return AutomaticDoorEffect::none;
    const auto* door=world.placement(index);
    if(!door || door->runtime_uid==0 || door->object!=349 ||
       door->source_frame>1u)
        return AutomaticDoorEffect::none;
    const auto* frame=source_door_point(door->source_animation,door->source_frame);
    if(!frame || !source_door_overlaps_player(*door,*frame,player))
        return AutomaticDoorEffect::none;
    const float entry_y=source_door_in_y(*door,*frame);
    if(!std::isfinite(entry_y))return AutomaticDoorEffect::none;
    const bool opening=door->source_frame==0 && player.y>=entry_y;
    const bool closing=door->source_frame==1 && player.y<entry_y;
    if(opening)
        return frame->metal?AutomaticDoorEffect::open_metal:
                            AutomaticDoorEffect::open_wood;
    if(closing)
        return frame->metal?AutomaticDoorEffect::close_metal:
                            AutomaticDoorEffect::close_wood;
    return AutomaticDoorEffect::none;
}

inline bool source_apply_auto_door_intention(
    WorldSystem& world, std::size_t index,
    std::uint64_t expected_uid,
    AutomaticDoorEffect effect) noexcept {
    if(effect==AutomaticDoorEffect::none || !world.generation_complete())
        return false;
    const auto* door=world.placement(index);
    if(!door || door->runtime_uid!=expected_uid ||
       door->object!=349)return false;
    const bool open=effect==AutomaticDoorEffect::open_wood ||
                    effect==AutomaticDoorEffect::open_metal;
    const auto required_frame=open?0u:1u;
    if(door->source_frame!=required_frame)return false;
    return apply_verified_door_frame_at_index(
        world,index,expected_uid,
        open?6061166023614368ULL:243854400993751ULL);
}

// Compatibility behavior for tests and direct original event dispatch. The
// gameplay scene instead queues the intention for 0.01 original seconds.
inline AutomaticDoorEffect source_auto_door_for_placement(
    WorldSystem& world, std::size_t index,
    WorldPoint player) noexcept {
    const auto effect=source_auto_door_intended_for_placement(
        world,index,player);
    const auto* door=world.placement(index);
    if(!door || !source_apply_auto_door_intention(
        world,index,door->runtime_uid,effect))
        return AutomaticDoorEffect::none;
    return effect;
}
// UID-based compatibility wrapper for event traces and native tests.
// The actual 3DS update loops over a precomputed door-placement index and
// must use the O(1) function above, never scan the whole 20k-object world
// once for every door in every rendered frame.
inline AutomaticDoorEffect source_auto_door_for_player(
    WorldSystem& world,std::uint64_t runtime_uid,
    WorldPoint player) noexcept {
    if(!runtime_uid)return AutomaticDoorEffect::none;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* p=world.placement(i);
        if(p && p->runtime_uid==runtime_uid)
            return source_auto_door_for_placement(world,i,player);
    }
    return AutomaticDoorEffect::none;
}
} // namespace minidayz::world_source
