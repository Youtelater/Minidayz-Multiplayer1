#include "minidayz/source_building_polygon_collision.hpp"
#include "minidayz/source_roof_interaction.hpp"
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
    // Build_mode(12) ghost Sprite573 frame12 uses ORIGINAL Construct
    // collision vertices; this is a deliberately inset nonrectangular
    // source mask, not the entire 37x21 frame bounding box.
    const auto* ghost=stage1::source_building_collision_polygon(573,0,12);
    assert(ghost && ghost->points==4);
    const auto* ghost_geom=stage1::source_building_frame_geometry(573,0,12);
    assert(ghost_geom && ghost_geom->width==37.0f &&
           ghost_geom->height==21.0f);
    assert(std::fabs(ghost_geom->origin_x-0.6f)<0.0001f);
    assert(std::fabs(ghost_geom->origin_y-0.7f)<0.0001f);
    const auto first=static_cast<std::size_t>(ghost->first)*2;
    assert(std::fabs(stage1::kSourceBuildingCollisionXY[first]-
                     (-0.3666666448f))<0.0001f);
    assert(std::fabs(stage1::kSourceBuildingCollisionXY[first+1]-
                     (-0.3199999928f))<0.0001f);
    // Original PlayerBase is a 16-vertex mask, not its enclosing square.
    WorldPlacement wall{};
    wall.object=346;wall.width=wall.height=0.1f;wall.scale_x=1.0f;
    wall.x=-1.05f;wall.y=-1.05f;
    assert(source_player_base_overlaps_wall(wall,{0,0}));
    wall.x=-1.99f;wall.y=-1.99f;
    assert(!source_player_base_overlaps_wall(wall,{0,0}));
    wall.x=-1.0f;wall.y=-1.0f;wall.angle=45.0f;
    assert(source_player_base_overlaps_wall(wall,{0,0}));
    wall.x=20.0f;
    assert(!source_player_base_overlaps_wall(wall,{0,0}));
    // Original 346 has no custom frame mask. Use actual transformed
    // overlap, independent of procedural parent and house UID.
    WorldPlacement zone{};
    zone.object=346;zone.width=zone.height=20.0f;zone.scale_x=1.0f;
    zone.origin_x=zone.origin_y=0.5f;
    WorldPlacement other=zone;
    other.x=4.0f;other.angle=37.0f;other.scale_x=-1.0f;
    assert(source_house_sprites_overlap(zone,other));
    assert(source_house_sprites_overlap(other,zone));
    other.x=100.0f;
    assert(!source_house_sprites_overlap(zone,other));
    other.x=0.0f;other.width=other.height=2.0f;
    assert(source_house_sprites_overlap(zone,other));
    assert(source_house_sprites_overlap(other,zone));
    other.width=0.0f;
    assert(!source_house_sprites_overlap(zone,other));
    WorldPlacement missing{};
    missing.object=9999;
    missing.width=missing.height=20.0f;
    bool known=false;
    assert(!source_building_polygon_contains(missing,{0.0f,0.0f},known));
    assert(!known);
    return 0;
}
