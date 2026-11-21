#pragma once
#include "minidayz/world.hpp"
#include "minidayz/generated/source_door_points.hpp"
#include <cmath>
#include <cstddef>

namespace minidayz::world_source {

// Original Doors_auto close UID 243854400993751 spawns Solid object 348 at
// object-349 Imagepoint 1, selects 348 frame 15, and rotates to Door.Angle+6.
// Open UID 6061166023614368 destroys that linked Solid. This helper models
// its *collision footprint* using the exact original 348-frame-15 polygon.
// It deliberately does not claim to execute Construct's UID/spawn/wait logic.
inline bool original_closed_door_solid_contains(
    const WorldPlacement& door, WorldPoint point,
    bool& supported) noexcept {
    supported=false;
    if (door.object!=349 || door.source_frame!=0 ||
        !std::isfinite(point.x) || !std::isfinite(point.y) ||
        !std::isfinite(door.x) || !std::isfinite(door.y) ||
        !std::isfinite(door.angle) || !(door.width>0.0f) ||
        !(door.height>0.0f)) return false;
    const auto* frame=source_door_point(
        door.source_animation,door.source_frame);
    if(!frame || !std::isfinite(frame->solid_anchor_x) ||
       !std::isfinite(frame->solid_anchor_y) ||
       door_solid348::kPolygonPointCount<3 ||
       !(door_solid348::kWidth>0.0f) ||
       !(door_solid348::kHeight>0.0f))return false;
    supported=true;
    constexpr float kRadians=0.01745329251994329577f;
    const float angle=door.angle*kRadians;
    const float ca=std::cos(angle),sa=std::sin(angle);
    const float mirror=door.scale_x<0.0f?-1.0f:1.0f;
    const float xx=door.width*(frame->solid_anchor_x-frame->origin_x)*mirror;
    const float yy=door.height*(frame->solid_anchor_y-frame->origin_y);
    const float cx=door.x+xx*ca-yy*sa;
    const float cy=door.y+xx*sa+yy*ca;
    // Command 84 after Spawn in the original source:
    // 348.SetAngle(349.Angle+6). Preserve its authored 6-degree offset.
    const float solid_angle=(door.angle+
        door_solid348::kDoorAngleOffsetDegrees)*kRadians;
    const float cs=std::cos(solid_angle),sn=std::sin(solid_angle);
    const float dx=point.x-cx,dy=point.y-cy;
    const float px=(dx*cs+dy*sn)/door_solid348::kWidth;
    const float py=(-dx*sn+dy*cs)/door_solid348::kHeight;
    if(!std::isfinite(px)||!std::isfinite(py))return false;
    // Source frame[10] vertices are hotspot-relative normalized pairs,
    // not the PNG sheet rectangle or pixel coordinates. Use the original
    // polygon instead of blocking the entire open doorway's sprite image.
    bool inside=false;
    const auto& vertices=door_solid348::kPolygonXY;
    constexpr auto count=door_solid348::kPolygonPointCount;
    for(std::size_t i=0,j=count-1;i<count;j=i++){
        const float xi=vertices[2*i],yi=vertices[2*i+1];
        const float xj=vertices[2*j],yj=vertices[2*j+1];
        const float vx=xj-xi,vy=yj-yi;
        const float cross=(px-xi)*vy-(py-yi)*vx;
        const float dot=(px-xi)*vx+(py-yi)*vy;
        const float len=vx*vx+vy*vy;
        if(std::fabs(cross)<0.0001f && dot>=-0.0001f &&
           dot<=len+0.0001f)return true;
        if ((yi>py)!=(yj>py)) {
            const float intercept=xi+(py-yi)*(xj-xi)/(yj-yi);
            if(px<intercept)inside=!inside;
        }
    }
    return inside;
}
} // namespace minidayz::world_source
