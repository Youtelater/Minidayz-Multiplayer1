#pragma once
#include "minidayz/world.hpp"
#include "minidayz/generated/source_roof_points.hpp"
#include <cmath>
#include <cstdint>

namespace minidayz::world_source {

// Root House_transparenting: object 1049 opacity 25% (event
// 3352196201835613), else opacity 100% (7755220865965526 and
// 9848716569903957). The parent event checks player_collision_base
// object 181 overlapping interior object 1041. The caller must supply
// that overlap; a visual roof alone never authorizes entry.
inline float source_house_roof_boundary_y(
    const WorldPlacement& roof,const SourceRoofPoint& sprite) noexcept {
    const float a=roof.angle*0.01745329251994329577f;
    const float dx=roof.width*(sprite.point_x-sprite.origin_x);
    const float dy=roof.height*(sprite.point_y-sprite.origin_y);
    return roof.y+dx*std::sin(a)+dy*std::cos(a);
}

// Conservative broadphase for the original 30x30 PlayerCollisionBase
// against an actual rotated Construct sprite rectangle. This is not
// polygon-precise picking; the SourceRoofPoint supplies exact artwork
// frame coordinates and imagepoint positions.
inline bool source_player_overlaps_house_rect(
    const WorldPlacement& rect,WorldPoint player,
    float origin_x,float origin_y) noexcept {
    if(!std::isfinite(player.x)||!std::isfinite(player.y)||
       !(rect.width>0.0f&&rect.height>0.0f))return false;
    const float a=rect.angle*0.01745329251994329577f;
    const float sn=std::sin(a),cs=std::cos(a);
    const float dx=player.x-rect.x,dy=player.y-rect.y;
    const float x=dx*cs+dy*sn,y=-dx*sn+dy*cs;
    const float half=15.0f*(std::fabs(cs)+std::fabs(sn));
    return x+half>=-rect.width*origin_x &&
           x-half<=rect.width*(1.0f-origin_x) &&
           y+half>=-rect.height*origin_y &&
           y-half<=rect.height*(1.0f-origin_y);
}

inline bool source_roof_opacity_at_player(
    const WorldPlacement& roof,WorldPoint player,
    bool overlaps_original_interior,float& opacity_out) noexcept {
    if(roof.object!=1049)return false;
    const auto* point=source_roof_point(roof.source_animation,roof.source_frame);
    if(!point)return false;
    // Original 181 overlapping 1049 and original 181 overlapping 1041
    // are separate event guards. Check both before transparency.
    const bool overlaps_roof=source_player_overlaps_house_rect(
        roof,player,point->origin_x,point->origin_y);
    const float boundary=source_house_roof_boundary_y(roof,*point);
    if(!std::isfinite(boundary))return false;
    opacity_out=overlaps_original_interior && overlaps_roof &&
                player.y<=boundary ? 0.25f : 1.0f;
    return true;
}
} // namespace minidayz::world_source
