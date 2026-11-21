#pragma once
#include "minidayz/world.hpp"
#include "minidayz/generated/source_family_members.hpp"
#include "minidayz/generated/source_roof_member_points.hpp"
#include <cmath>

namespace minidayz::world_source {

// Root House_transparenting changes the family 1049 members' opacity to
// 25% / 100% when the player collision sprite (181) overlaps a family
// 1041 interior and crosses the roof member's ImagePointY(1).
// Families 1041/1049 have NO own sprite frames; resolve their real
// source member object IDs before evaluating either original condition.
inline bool source_player_overlaps_world_rect(
    const WorldPlacement& object,WorldPoint player,
    float origin_x,float origin_y) noexcept {
    if(!std::isfinite(player.x)||!std::isfinite(player.y)||
       !(object.width>0.0f&&object.height>0.0f))return false;
    const float a=object.angle*0.01745329251994329577f;
    const float sn=std::sin(a),cs=std::cos(a);
    const float dx=player.x-object.x,dy=player.y-object.y;
    const float local_x=dx*cs+dy*sn,local_y=-dx*sn+dy*cs;
    // Original player_collision_base sprite is 30x30.
    const float half=15.0f*(std::fabs(cs)+std::fabs(sn));
    return local_x+half>=-object.width*origin_x &&
           local_x-half<=object.width*(1.0f-origin_x) &&
           local_y+half>=-object.height*origin_y &&
           local_y-half<=object.height*(1.0f-origin_y);
}

inline float source_roof_in_world_y(
    const WorldPlacement& roof,const SourceRoofMemberPoint& point) noexcept {
    const float a=roof.angle*0.01745329251994329577f;
    const float dx=(point.point_x-point.origin_x)*roof.width;
    const float dy=(point.point_y-point.origin_y)*roof.height;
    return roof.y+dx*std::sin(a)+dy*std::cos(a);
}

inline bool source_roof_opacity_at_player(
    const WorldPlacement& roof,WorldPoint player,
    bool overlaps_original_house_interior,float& result) noexcept {
    if(!stage1::source_family_contains(1049,roof.object))return false;
    const auto* frame=source_roof_member_point(
        roof.object,roof.source_animation,roof.source_frame);
    if(!frame)return false;
    const bool player_under_roof=source_player_overlaps_world_rect(
        roof,player,frame->origin_x,frame->origin_y);
    const float threshold=source_roof_in_world_y(roof,*frame);
    if(!std::isfinite(threshold))return false;
    // Event 3352196201835613: 25%, when Y <= ImagePointY(1).
    // Event 7755220865965526 / 9848716569903957: restore 100%.
    result=overlaps_original_house_interior && player_under_roof &&
           player.y<=threshold?0.25f:1.0f;
    return true;
}
} // namespace minidayz::world_source
