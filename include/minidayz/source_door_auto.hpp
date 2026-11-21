#pragma once
#include "minidayz/world.hpp"
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
// Native overlap is a rotated-rectangle approximation of Construct sprite
// collision, *not* a claim to reproduce its polygon/obstacle-base conditions.
// The source frame point and source audio family are exact.
enum class AutomaticDoorEffect : std::uint8_t {
    none, open_wood, open_metal, close_wood, close_metal
};

inline bool source_door_overlaps_player(
    const WorldPlacement& door, const SourceDoorPoint& frame,
    WorldPoint player) noexcept {
    if (!std::isfinite(player.x) || !std::isfinite(player.y) ||
        door.width<=0.0f || door.height<=0.0f) return false;
    const float a=door.angle*0.01745329251994329577f;
    const float sn=std::sin(a),cs=std::cos(a);
    const float dx=player.x-door.x,dy=player.y-door.y;
    const float px=dx*cs+dy*sn,py=-dx*sn+dy*cs;
    // Original player_collision_base (object 181) is a 30x30 sprite.
    // Project its axis-aligned AABB onto door-local axes conservatively.
    const float half=15.0f*(std::fabs(cs)+std::fabs(sn));
    const float min_x=-door.width*frame.origin_x;
    const float max_x=door.width*(1.0f-frame.origin_x);
    const float min_y=-door.height*frame.origin_y;
    const float max_y=door.height*(1.0f-frame.origin_y);
    return px+half>=min_x && px-half<=max_x &&
           py+half>=min_y && py-half<=max_y;
}

inline float source_door_in_y(const WorldPlacement& door,
                             const SourceDoorPoint& frame) noexcept {
    const float a=door.angle*0.01745329251994329577f;
    const float dx=door.width*(frame.in_x-frame.origin_x);
    const float dy=door.height*(frame.in_y-frame.origin_y);
    return door.y+dx*std::sin(a)+dy*std::cos(a);
}

// Source-gated door state transition. The native caller must only run this
// for a live player and a generated Map: menu, death and pause do not animate
// doors. It returns a sound cue only on an actual successful frame change.
inline AutomaticDoorEffect source_auto_door_for_placement(
    WorldSystem& world, std::size_t index,
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
    if(!opening && !closing)return AutomaticDoorEffect::none;
    const auto event_uid=opening ? 6061166023614368ULL : 243854400993751ULL;
    const bool metal=frame->metal;
    if(!apply_verified_door_frame_at_index(
            world,index,door->runtime_uid,event_uid))
        return AutomaticDoorEffect::none;
    if(opening)
        return metal?AutomaticDoorEffect::open_metal:AutomaticDoorEffect::open_wood;
    return metal?AutomaticDoorEffect::close_metal:AutomaticDoorEffect::close_wood;
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
