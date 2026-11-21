#pragma once
#include "minidayz/world.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include "minidayz/generated/source_door_points.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace minidayz::terrain {

// Source Map static collision broadphase for Nintendo 3DS. Before this index
// every joystick movement performed 8 * placement_count() expensive string
// lookups and rotated hit tests, making the original game feel like ~10 FPS.
// Actual narrow-phase geometry is still tested by the caller.
class SourceObstacleIndex {
public:
    static constexpr float kCell=256.0f;
    void rebuild(const WorldSystem& world) {
        const auto& bounds=world.bounds();
        cols_=std::max(1,static_cast<int>(std::ceil(bounds.width/kCell)));
        rows_=std::max(1,static_cast<int>(std::ceil(bounds.height/kCell)));
        buckets_.clear();
        buckets_.resize(static_cast<std::size_t>(cols_)*rows_);
        const auto* layout=stage1::find_layout("Map");
        if(!layout)return;
        const auto layout_index=static_cast<std::uint16_t>(
            layout-stage1::kLayoutTable);
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(!p||p->width<=0||p->height<=0||
               source_visual_roof_overlay(p->object))continue;
            const auto* object=stage1::find_object(p->object);
            if(object&&object->type==16)continue; // Tilemap solids are elsewhere
            const auto* layer=stage1::find_layer(layout_index,p->layer);
            if(!layer)continue;
            const bool building=std::strcmp(layer->name,"Buildings")==0;
            // The original procedural world creates the invisible
            // obstacle-base sprites on Obstacle_enviroment, independently
            // of their visible Trees / Forests artwork. This layer is
            // solid and must participate in broadphase candidate lookup.
            const bool barrier=std::strcmp(layer->name,"Obstacles")==0 ||
                               std::strcmp(layer->name,"Obstacle_enviroment")==0 ||
                               std::strcmp(layer->name,"Water")==0;
            if(!building&&!barrier)continue;
            // The Buildings layer includes decorative floors and roofs.
            // Only original Construct Solid sprites (or door 349 with
            // its separate Solid-348 companion) enter collision broadphase.
            if(building && p->object!=349 &&
               !stage1::source_object_has_solid_behavior(p->object))continue;
            // Conservative circumscribed AABB covers every rotation and
            // hotspot, without producing false negatives at the border.
            const float center_x=p->x;
            const float center_y=p->y;
            // Rotation is around the Construct hotspot, not the centre.
            // hypot(w,h) encloses every possible hotspot-relative corner.
            // Object-349 is decorative door artwork; its original closed
            // obstacle is Solid object-348 spawned at Imagepoint 1 with
            // frame 15. Index the entire possible companion footprint;
            // otherwise a movement query can miss it even though polygon
            // collision below uses the correct source frame.
            const float radius=std::hypot(p->width,p->height)+
                (p->object==349
                    ? std::hypot(world_source::door_solid348::kWidth,
                                 world_source::door_solid348::kHeight)
                    : 0.0f);
            const int x0=std::clamp(static_cast<int>(
                std::floor((center_x-radius)/kCell)),0,cols_-1);
            const int y0=std::clamp(static_cast<int>(
                std::floor((center_y-radius)/kCell)),0,rows_-1);
            const int x1=std::clamp(static_cast<int>(
                std::floor((center_x+radius)/kCell)),0,cols_-1);
            const int y1=std::clamp(static_cast<int>(
                std::floor((center_y+radius)/kCell)),0,rows_-1);
            for(int y=y0;y<=y1;++y)
                for(int x=x0;x<=x1;++x)
                    buckets_[static_cast<std::size_t>(y)*cols_+x]
                        .push_back(static_cast<std::uint32_t>(i));
        }
    }
    const std::vector<std::uint32_t>& candidates(float x,float y) const noexcept {
        static const std::vector<std::uint32_t> empty{};
        if(buckets_.empty()||!std::isfinite(x)||!std::isfinite(y)||
            x<0||y<0)return empty;
        const int cx=static_cast<int>(x/kCell);
        const int cy=static_cast<int>(y/kCell);
        if(cx>=cols_||cy>=rows_)return empty;
        return buckets_[static_cast<std::size_t>(cy)*cols_+cx];
    }
private:
    int cols_{},rows_{};
    std::vector<std::vector<std::uint32_t>> buckets_{};
};
} // namespace minidayz::terrain
