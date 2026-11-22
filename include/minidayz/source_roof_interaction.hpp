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
    float origin_x,float origin_y,float player_half=15.0f) noexcept {
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

// Original t193 PlayerBase mask, frame[10], from root Construct data.
// t346 warmzone has no custom mask; its authored transformed rectangle
// is tested against this mask, not the 2x2 bounding box.
inline bool source_player_base_overlaps_wall(
    const WorldPlacement& wall,WorldPoint player) noexcept {
    if(!(wall.width>0.0f && wall.height>0.0f) ||
       !std::isfinite(wall.angle) || !std::isfinite(player.x) ||
       !std::isfinite(player.y))return false;
    constexpr float xy[]={-0.833333015f,-0.833333015f,-0.682291985f,-0.947916508f,-0.500000000f,-1.00000000f,-0.302084029f,-0.947916508f,-0.166666985f,-0.833333015f,-0.0572919846f,-0.687500000f,0.00000000f,-0.500000000f,-0.0625000596f,-0.302084029f,-0.166665971f,-0.166666985f,-0.312500060f,-0.0625000596f,-0.500000000f,0.00000000f,-0.682291985f,-0.0572919846f,-0.833333015f,-0.166666985f,-0.947916508f,-0.322916985f,-1.00000000f,-0.500000000f,-0.947916508f,-0.682291985f};
    const float a=wall.angle*0.01745329251994329577f;
    const float cs=std::cos(a),sn=std::sin(a);
    const float mirror=wall.scale_x<0.0f?-1.0f:1.0f;
    const float cx=(0.5f-wall.origin_x)*wall.width;
    const float cy=(0.5f-wall.origin_y)*wall.height;
    return source_polygon_overlaps_box(sizeof(xy)/sizeof(float)/2,
        [&](std::size_t i){
            const float x=player.x+2.0f*xy[2*i]-wall.x;
            const float y=player.y+2.0f*xy[2*i+1]-wall.y;
            return WorldPoint{(x*cs+y*sn)*mirror-cx,
                              -x*sn+y*cs-cy};
        },wall.width*0.5f,wall.height*0.5f);
}

// Construct overlap picking uses the active frame mask on both instances.
// Source frame[10] coordinates are hotspot-relative; default masks use
// the full original frame rectangle. Handle concave masks and mirroring.
inline std::size_t source_house_mask_count(const WorldPlacement& p) noexcept {
    const auto* m=stage1::source_building_collision_polygon(
        p.object,p.source_animation,p.source_frame);
    return m && m->points>=3?m->points:4;
}
inline WorldPoint source_house_mask_vertex(const WorldPlacement& p,std::size_t i) noexcept {
    const auto* m=stage1::source_building_collision_polygon(
        p.object,p.source_animation,p.source_frame);
    const float x=m && m->points>=3?
        stage1::kSourceBuildingCollisionXY[2*(m->first+i)]:
        ((i==1 || i==2)?1.0f:0.0f)-p.origin_x;
    const float y=m && m->points>=3?
        stage1::kSourceBuildingCollisionXY[2*(m->first+i)+1]:
        (i>=2?1.0f:0.0f)-p.origin_y;
    const float a=p.angle*0.01745329251994329577f;
    const float dx=x*p.width*(p.scale_x<0?-1.0f:1.0f),dy=y*p.height;
    return {p.x+dx*std::cos(a)-dy*std::sin(a),
            p.y+dx*std::sin(a)+dy*std::cos(a)};
}
inline bool source_house_sprites_overlap(const WorldPlacement& a,
                                         const WorldPlacement& b) noexcept {
    if(!(a.width>0 && a.height>0 && b.width>0 && b.height>0) ||
       !std::isfinite(a.x) || !std::isfinite(a.y) ||
       !std::isfinite(b.x) || !std::isfinite(b.y) ||
       !std::isfinite(a.angle) || !std::isfinite(b.angle))return false;
    const auto na=source_house_mask_count(a),nb=source_house_mask_count(b);
    auto cross=[](WorldPoint p,WorldPoint q,WorldPoint r){
        return (q.x-p.x)*(r.y-p.y)-(q.y-p.y)*(r.x-p.x);
    };
    auto contains=[](const WorldPlacement& p,std::size_t n,WorldPoint q){
        bool inside=false;auto prev=source_house_mask_vertex(p,n-1);
        for(std::size_t i=0;i<n;++i){
            const auto cur=source_house_mask_vertex(p,i);
            if((cur.y>q.y)!=(prev.y>q.y) &&
               q.x<cur.x+(q.y-cur.y)*(prev.x-cur.x)/(prev.y-cur.y))
                inside=!inside;
            prev=cur;
        }
        return inside;
    };
    if(contains(a,na,source_house_mask_vertex(b,0)) ||
       contains(b,nb,source_house_mask_vertex(a,0)))return true;
    for(std::size_t i=0;i<na;++i){
        const auto p=source_house_mask_vertex(a,i),q=source_house_mask_vertex(a,(i+1)%na);
        for(std::size_t j=0;j<nb;++j){
            const auto r=source_house_mask_vertex(b,j),s=source_house_mask_vertex(b,(j+1)%nb);
            if(std::max(p.x,q.x)<std::min(r.x,s.x) ||
               std::max(r.x,s.x)<std::min(p.x,q.x) ||
               std::max(p.y,q.y)<std::min(r.y,s.y) ||
               std::max(r.y,s.y)<std::min(p.y,q.y))continue;
            const float c1=cross(p,q,r),c2=cross(p,q,s),
                        c3=cross(r,s,p),c4=cross(r,s,q);
            if(((c1<=0 && c2>=0)||(c1>=0 && c2<=0)) &&
               ((c3<=0 && c4>=0)||(c3>=0 && c4<=0)))return true;
        }
    }
    return false;
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
