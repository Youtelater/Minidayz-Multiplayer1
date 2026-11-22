#include "minidayz/source_building_polygon_collision.hpp"
#include "minidayz/generated/source_building_frame_geometry.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <initializer_list>

int main(){
    using namespace minidayz;
    using namespace minidayz::world_source;
    assert(stage1::source_object_has_solid_behavior(348));
    assert(stage1::kSourceBuildingCollisionPolygonCount>0);
    assert(stage1::kSourceBuildingFrameGeometryCount>=
           stage1::kSourceBuildingCollisionPolygonCount);
    for(const auto& polygon:stage1::kSourceBuildingCollisionPolygons){
        const auto* frame=stage1::source_building_frame_geometry(
            polygon.object,polygon.animation,polygon.frame);
        assert(frame && frame->width>0.0f && frame->height>0.0f);
        assert(std::isfinite(frame->origin_x)&&std::isfinite(frame->origin_y));
    }
    constexpr float radians=0.01745329251994329577f;
    const std::size_t count=stage1::kSourceBuildingCollisionPolygonCount;
    for(std::size_t i=0;i<count;++i){
        const auto& f=stage1::kSourceBuildingCollisionPolygons[i];
        assert(f.points>=3);
        // Both Solid and non-Solid source sprites have authored masks.
        // Non-Solid masks are used by interactions, not movement blocking.
        WorldPlacement p{};
        p.object=f.object;
        p.source_animation=f.animation;
        p.source_frame=f.frame;
        p.x=1200.0f;p.y=1400.0f;
        const auto* geometry=stage1::source_building_frame_geometry(
            f.object,f.animation,f.frame);
        assert(geometry);
        p.width=geometry->width;p.height=geometry->height;
        p.origin_x=geometry->origin_x;p.origin_y=geometry->origin_y;
        bool known=false;
        for(float angle:{0.0f,90.0f,147.0f}){
            p.angle=angle;
            const float a=angle*radians;
            for(float mirror:{1.0f,-1.0f}){
                p.scale_x=mirror;
                // A boundary vertex from the root original must still
                // belong to the polygon after rotation and mirroring.
                const auto offset=static_cast<std::size_t>(f.first)*2;
                const float vx=stage1::kSourceBuildingCollisionXY[offset]*
                    p.width*mirror;
                const float vy=stage1::kSourceBuildingCollisionXY[offset+1]*
                    p.height;
                const WorldPoint pt{
                    p.x+vx*std::cos(a)-vy*std::sin(a),
                    p.y+vx*std::sin(a)+vy*std::cos(a)};
                assert(source_building_polygon_contains(p,pt,known));
                assert(known);
                assert(!source_building_polygon_contains(
                    p,{p.x+10000.0f,p.y+10000.0f},known));
                assert(known);
            }
        }
    }
    WorldPlacement missing{};
    missing.object=9999;
    missing.width=missing.height=20.0f;
    bool known=false;
    assert(!source_building_polygon_contains(missing,{0.0f,0.0f},known));
    assert(!known);
    return 0;
}
