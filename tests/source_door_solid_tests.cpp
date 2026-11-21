#include "minidayz/source_door_solid.hpp"
#include <cassert>
#include <cmath>

namespace {
minidayz::WorldPoint original_solid_vertex_world(
    const minidayz::WorldPlacement& door,
    const minidayz::world_source::SourceDoorPoint& frame,
    std::size_t vertex) {
    constexpr float rad=0.01745329251994329577f;
    const float a=door.angle*rad;
    const float mx=door.scale_x<0.0f?-1.0f:1.0f;
    const float dx=(frame.solid_anchor_x-frame.origin_x)*door.width*mx;
    const float dy=(frame.solid_anchor_y-frame.origin_y)*door.height;
    const float x=door.x+dx*std::cos(a)-dy*std::sin(a);
    const float y=door.y+dx*std::sin(a)+dy*std::cos(a);
    const float b=(door.angle+6.0f)*rad;
    const float vx=minidayz::world_source::door_solid348::kPolygonXY[vertex*2]*
                   minidayz::world_source::door_solid348::kWidth;
    const float vy=minidayz::world_source::door_solid348::kPolygonXY[vertex*2+1]*
                   minidayz::world_source::door_solid348::kHeight;
    return {x+vx*std::cos(b)-vy*std::sin(b),
            y+vx*std::sin(b)+vy*std::cos(b)};
}
}
int main() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    assert(door_solid348::kObject==348);
    assert(door_solid348::kClosedFrame==15);
    assert(door_solid348::kPolygonPointCount>=3);
    assert(door_solid348::kWidth>0.0f);
    assert(door_solid348::kHeight>0.0f);
    const SourceDoorPoint* frame=nullptr;
    for(const auto& source:kSourceDoorPoints){
        if(source.frame==0){frame=&source;break;}
    }
    assert(frame);
    WorldPlacement door{};
    door.object=349;
    door.source_animation=frame->animation;
    door.source_frame=0;
    door.x=1000.0f;
    door.y=3000.0f;
    door.width=frame->width;
    door.height=frame->height;
    door.origin_x=frame->origin_x;
    door.origin_y=frame->origin_y;
    door.angle=24.0f;
    for(float mirror: {1.0f,-1.0f}){
        door.scale_x=mirror;
        for(std::size_t v=0;v<door_solid348::kPolygonPointCount;++v){
            const WorldPoint point=original_solid_vertex_world(door,*frame,v);
            bool supported=false;
            assert(original_closed_door_solid_contains(door,point,supported));
            assert(supported);
        }
        bool supported=false;
        assert(!original_closed_door_solid_contains(
            door,{door.x+2000.0f,door.y+2000.0f},supported));
        assert(supported);
    }
    // Opening the door destroys the original 348 Solid collider.
    door.source_frame=1;
    bool supported=false;
    assert(!original_closed_door_solid_contains(
        door,{door.x,door.y},supported));
    assert(!supported);
    return 0;
}
