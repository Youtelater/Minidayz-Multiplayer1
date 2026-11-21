#pragma once
#include "minidayz/world.hpp"
#include "minidayz/source_polygon_overlap.hpp"
#include "minidayz/generated/source_building_collision_polygons.hpp"
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
    float origin_x,float origin_y,float player_half=15.0f) noexcept {
    if(!std::isfinite(player.x)||!std::isfinite(player.y)||
       !(object.width>0.0f&&object.height>0.0f))return false;
    const float a=object.angle*0.01745329251994329577f;
    const float sn=std::sin(a),cs=std::cos(a);
    const float dx=player.x-object.x,dy=player.y-object.y;
    float local_x=dx*cs+dy*sn;
    const float local_y=-dx*sn+dy*cs;
    // Construct permits horizontally mirrored roof/interior sprites.
    // Width is already the authored rendered width: only flip local X.
    if(object.scale_x<0.0f)local_x=-local_x;
    // Original player_collision_base sprite is 30x30.
    const float half=player_half*(std::fabs(cs)+std::fabs(sn));
    if (!(local_x+half>=-object.width*origin_x &&
          local_x-half<=object.width*(1.0f-origin_x) &&
          local_y+half>=-object.height*origin_y &&
          local_y-half<=object.height*(1.0f-origin_y))) return false;
    // Complete rectangle SAT: local axes alone admit false overlap
    // at rotated corners. Also test the world-aligned player axes.
    const float center_x=(0.5f-origin_x)*object.width*
                         (object.scale_x<0.0f?-1.0f:1.0f);
    const float center_y=(0.5f-origin_y)*object.height;
    const float world_x=object.x+center_x*cs-center_y*sn;
    const float world_y=object.y+center_x*sn+center_y*cs;
    const float extent_x=0.5f*(object.width*std::fabs(cs)+object.height*std::fabs(sn));
    const float extent_y=0.5f*(object.width*std::fabs(sn)+object.height*std::fabs(cs));
    return std::fabs(player.x-world_x)<=player_half+extent_x &&
           std::fabs(player.y-world_y)<=player_half+extent_y;
}

// The original Overlaps conditions use the active Sprite mask even for
// non-Solid objects. Use its authored vertices where present and the
// default full-frame rectangle otherwise. Player remains a 30x30 box.
inline bool source_player_overlaps_world_sprite(
    const WorldPlacement& object,WorldPoint player,
    float origin_x,float origin_y) noexcept {
    if(!std::isfinite(player.x)||!std::isfinite(player.y)||
       !std::isfinite(object.x)||!std::isfinite(object.y)||
       !std::isfinite(object.angle)||!std::isfinite(object.width)||
       !std::isfinite(object.height)||!(object.width>0.0f&&object.height>0.0f))
        return false;
    const auto* mask=stage1::source_building_collision_polygon(
        object.object,object.source_animation,object.source_frame);
    if(!mask || mask->points<3)
        return source_player_overlaps_world_rect(object,player,origin_x,origin_y,player_half);
    const float a=object.angle*0.01745329251994329577f;
    const float cs=std::cos(a),sn=std::sin(a);
    const float mirror=object.scale_x<0.0f?-1.0f:1.0f;
    const auto* xy=stage1::kSourceBuildingCollisionXY+2*mask->first;
    return source_polygon_overlaps_box(mask->points,[&](std::size_t i){
        const float x=xy[2*i]*object.width*mirror;
        const float y=xy[2*i+1]*object.height;
        return WorldPoint{object.x-player.x+x*cs-y*sn,
                          object.y-player.y+x*sn+y*cs};
    },player_half,player_half);
}

inline float source_roof_in_world_y(
    const WorldPlacement& roof,const SourceRoofMemberPoint& point) noexcept {
    const float a=roof.angle*0.01745329251994329577f;
    const float dx=(point.point_x-point.origin_x)*roof.width*
                   (roof.scale_x<0.0f?-1.0f:1.0f);
    const float dy=(point.point_y-point.origin_y)*roof.height;
    return roof.y+dx*std::sin(a)+dy*std::cos(a);
}

inline float source_roof_depth_world_y(
    const WorldPlacement& roof,const SourceRoofMemberPoint& point) noexcept {
    const float a=roof.angle*0.01745329251994329577f;
    const float dx=(point.depth_x-point.origin_x)*roof.width*
                   (roof.scale_x<0.0f?-1.0f:1.0f);
    const float dy=(point.depth_y-point.origin_y)*roof.height;
    return roof.y+dx*std::sin(a)+dy*std::cos(a);
}
struct SourceRoofDecision {
    float opacity{1.0f};
    bool move_background{true};
    bool foundation_top{};
    bool under_roof{};
};
// Source House_transparenting: 181 overlap roof 1049, then distinct
// PlayerBase 193 wall-zone 346 guard. Imagepoints 1 and 2 are different
// thresholds: opacity and layer respectively. A picked foundation is
// selected by source position/association, NOT player-vs-Solid overlap.
inline SourceRoofDecision source_house_roof_decision(
    const WorldPlacement& roof, const SourceRoofMemberPoint& frame,
    WorldPoint player, bool picked_foundation, bool touching_wall) noexcept {
    SourceRoofDecision result;
    result.under_roof=source_player_overlaps_world_sprite(
        roof,player,frame.origin_x,frame.origin_y);
    if(!result.under_roof)return result;
    if(touching_wall && picked_foundation){
        result.opacity=0.0f;
        return result;
    }
    if(touching_wall)return result;
    const float opacity_y=source_roof_in_world_y(roof,frame);
    const float depth_y=source_roof_depth_world_y(roof,frame);
    if(std::isfinite(depth_y))result.move_background=player.y>depth_y;
    if(picked_foundation && std::isfinite(opacity_y) && player.y<=opacity_y){
        result.opacity=0.25f;
        result.foundation_top=true;
    }
    return result;
}

inline bool source_roof_opacity_at_player(
    const WorldPlacement& roof,WorldPoint player,
    bool overlaps_original_house_interior,float& result) noexcept {
    if(!stage1::source_family_contains(1049,roof.object))return false;
    const auto* frame=source_roof_member_point(
        roof.object,roof.source_animation,roof.source_frame);
    if(!frame)return false;
    const bool player_under_roof=source_player_overlaps_world_sprite(
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
