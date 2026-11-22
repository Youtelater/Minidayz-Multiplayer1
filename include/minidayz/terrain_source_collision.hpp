#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include "minidayz/generated/source_family_members.hpp"

namespace minidayz::terrain {

// Root Game_events > House_transparenting (UID 5249390718058362)
// changes the opacity and draw layer of roof object 1049. This is a
// visibility overlay, not a solid wall. In the native port, applying
// a blanket "all Buildings sprites are solid" rule to a roof would
// make the house impossible to enter even after the roof is faded.
// Other building sprites still require their source collision tests.
inline bool source_visual_roof_overlay(
    std::uint16_t object_id) noexcept {
    // Construct 1049 is a FAMILY, not an actual Sprite object. Check its
    // exact member list from root project[4]; testing object==1049 skipped
    // every real house-roof PNG and wrongly made roofs solid walls.
    return stage1::source_family_contains(1049,object_id);
}


// Original create_map_element tree branches (e.g. UID 5221960099677299)
// spawn an invisible obst_base Sprite345 companion onto "zed_resp".
// It is NOT scenery and does NOT belong on Obstacle_enviroment. Only
// a linked runtime tree companion is solid: generic zed_resp markers
// must never block the player.
inline bool source_linked_tree_collision_hitbox(std::uint16_t object,
                                                std::uint64_t tree_uid,
                                                const char* layer) noexcept {
    return object==345 && tree_uid!=0 && layer &&
           std::strcmp(layer,"zed_resp")==0;
}

// The original Solid behavior is independent of rendering layer. House
// event 4153213040695326 moves a foundation to Buildings_base_top and
// 7058744064425045 moves it back. Neither action disables Solid.
inline bool source_building_collision_layer(const char* layer,
                                             std::uint16_t object) noexcept {
    return (layer && (std::strcmp(layer,"Buildings")==0 ||
            std::strcmp(layer,"Buildings_bg")==0 ||
            std::strcmp(layer,"Buildings_base")==0 ||
            std::strcmp(layer,"Buildings_base_top")==0 ||
            std::strcmp(layer,"Buildings_shadows")==0)) ||
           object==348 || stage1::source_family_contains(1048,object) ||
           stage1::source_family_contains(1041,object);
}

// Original Construct world placements store a hotspot, source dimensions,
// and an angle in degrees. Evaluate the true rotated rectangle rather than
// its unrotated bounding box (which blocks doorways beside slanted walls).
inline bool source_solid_sprite_contains(
    float hot_x,float hot_y,float width,float height,
    float origin_x,float origin_y,float angle_degrees,
    float point_x,float point_y,float scale_x=1.0f) noexcept {
    if (!(width>0.0f&&height>0.0f) ||
        !std::isfinite(hot_x)||!std::isfinite(hot_y)||
        !std::isfinite(origin_x)||!std::isfinite(origin_y)||
        !std::isfinite(angle_degrees)||!std::isfinite(point_x)||
        !std::isfinite(point_y)) return false;
    // Construct rotates the rectangle AROUND its image hotspot, not
    // around the top-left corner. Previously the inverse transform was
    // applied after a top-left translation, which moved rotated house
    // collision rectangles away from their real walls and doorways.
    // Convert the query to hotspot-relative coordinates FIRST, undo the
    // rotation, then add the original frame's hotspot offset.
    const float dx=point_x-hot_x,dy=point_y-hot_y;
    float local_x=dx,local_y=dy;
    if(angle_degrees!=0.0f){
        constexpr float radians=0.01745329251994329577f;
        const float angle=angle_degrees*radians;
        const float sn=std::sin(angle),cs=std::cos(angle);
        local_x=dx*cs+dy*sn;
        local_y=-dx*sn+dy*cs;
    }
    // A negative Construct scale_x flips the original sprite horizontally
    // around its hotspot. The instance width already includes its size.
    if(scale_x<0.0f)local_x=-local_x;
    const float px=local_x+width*origin_x;
    const float py=local_y+height*origin_y;
    return px>=0.0f&&py>=0.0f&&px<=width&&py<=height;
}
}
