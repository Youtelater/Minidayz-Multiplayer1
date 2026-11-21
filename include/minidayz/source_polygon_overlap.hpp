#pragma once
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace minidayz::world_source {
// Exact intersection of a simple authored sprite polygon and an axis-aligned
// player box. Vertices are world-space relative to the player center.
// Works for concave polygons; no convexity assumption or heap allocation.
template<class Vertex>
inline bool source_polygon_overlaps_box(std::size_t count,Vertex vertex,
                                        float half_x,float half_y) noexcept {
    if(count<3 || !(half_x>0) || !(half_y>0))return false;
    bool inside=false;
    auto prev=vertex(count-1);
    for(std::size_t i=0;i<count;++i){
        auto cur=vertex(i);
        if(!std::isfinite(cur.x)||!std::isfinite(cur.y) ||
           !std::isfinite(prev.x)||!std::isfinite(prev.y))return false;
        if(std::fabs(cur.x)<=half_x && std::fabs(cur.y)<=half_y)return true;
        // Liang-Barsky slab test for each polygon edge against the box.
        float lo=0.0f,hi=1.0f;
        auto clip=[&](float p,float d,float h){
            if(d==0.0f)return std::fabs(p)<=h;
            float a=(-h-p)/d,b=(h-p)/d;
            if(a>b)std::swap(a,b);
            lo=std::max(lo,a);hi=std::min(hi,b);
            return lo<=hi;
        };
        if(clip(prev.x,cur.x-prev.x,half_x) &&
           clip(prev.y,cur.y-prev.y,half_y))return true;
        // If no edge touches the player box, it may lie wholly inside
        // the polygon; odd-even center containment suffices in that case.
        if((cur.y>0)!=(prev.y>0)){
            const float x=cur.x-cur.y*(prev.x-cur.x)/(prev.y-cur.y);
            if(x>0)inside=!inside;
        }
        prev=cur;
    }
    return inside;
}
} // namespace minidayz::world_source
