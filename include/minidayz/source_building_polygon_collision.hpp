#pragma once
#include "minidayz/world.hpp"
#include "minidayz/generated/source_building_collision_polygons.hpp"
#include <cmath>
#include <cstddef>

namespace minidayz::world_source {

// Test exactly the root Construct Sprite collision polygon at the active
// animation/frame, using the rendered instance width/height, hotspot,
// rotation and horizontal mirror. If no custom frame polygon exists, the
// caller uses the ordinary Construct default frame rectangle.
inline bool source_building_polygon_contains(
    const WorldPlacement& placement, WorldPoint query,
    bool& has_custom_polygon) noexcept {
    has_custom_polygon=false;
    if(!std::isfinite(query.x)||!std::isfinite(query.y) ||
       !std::isfinite(placement.x)||!std::isfinite(placement.y) ||
       !std::isfinite(placement.angle) ||
       !(placement.width>0.0f) || !(placement.height>0.0f))
        return false;
    const auto* frame=stage1::source_building_collision_polygon(
        placement.object,placement.source_animation,placement.source_frame);
    if(!frame || frame->points<3)return false;
    has_custom_polygon=true;
    constexpr float radians=0.01745329251994329577f;
    const float a=placement.angle*radians;
    const float cs=std::cos(a),sn=std::sin(a);
    const float dx=query.x-placement.x,dy=query.y-placement.y;
    const float mirror=placement.scale_x<0.0f?-1.0f:1.0f;
    const float px=(dx*cs+dy*sn)*mirror/placement.width;
    const float py=(-dx*sn+dy*cs)/placement.height;
    if(!std::isfinite(px)||!std::isfinite(py))return false;
    const auto* xy=stage1::kSourceBuildingCollisionXY+2*frame->first;
    const std::size_t count=frame->points;
    bool inside=false;
    for(std::size_t i=0,j=count-1;i<count;j=i++){
        const float xi=xy[2*i],yi=xy[2*i+1];
        const float xj=xy[2*j],yj=xy[2*j+1];
        const float vx=xj-xi,vy=yj-yi;
        const float cross=(px-xi)*vy-(py-yi)*vx;
        const float dot=(px-xi)*vx+(py-yi)*vy;
        const float len=vx*vx+vy*vy;
        if(len>0.00000001f && std::fabs(cross)<0.0001f && dot>=-0.0001f &&
           dot<=len+0.0001f)return true;
        if((yi>py)!=(yj>py)){
            const float intersection=xi+(py-yi)*(xj-xi)/(yj-yi);
            if(px<intersection)inside=!inside;
        }
    }
    return inside;
}
} // namespace minidayz::world_source
