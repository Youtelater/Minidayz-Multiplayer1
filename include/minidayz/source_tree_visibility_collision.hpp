#pragma once
#include "minidayz/world.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include "minidayz/generated/source_tree_impact_points.hpp"
#include <algorithm>
#include <cmath>

namespace minidayz::terrain {

// Original six tree sprite species 184/207/209/576/577/581 create a
// separate 345 frame10 at their *Imagepoint 1*. This pixel footprint is
// the trunk. The canopy PNG is never a full rectangular solid.
inline bool source_standing_tree_trunk_position(
    const WorldPlacement& tree,float& x,float& y) noexcept {
    const auto* point=world_source::source_tree_impact_point(tree.object);
    if(!point||tree.source_frame!=0||tree.opacity<=0.0f)return false;
    const float width=tree.width>0.0f?tree.width:point->width;
    const float height=tree.height>0.0f?tree.height:point->height;
    if(!(width>0.0f&&height>0.0f))return false;
    constexpr float kRad=0.01745329251994329577f;
    const float angle=tree.angle*kRad;
    const float local_x=(point->point_x-point->origin_x)*width*
                        (tree.scale_x<0.0f?-1.0f:1.0f);
    const float local_y=(point->point_y-point->origin_y)*height;
    x=tree.x+local_x*std::cos(angle)-local_y*std::sin(angle);
    y=tree.y+local_x*std::sin(angle)+local_y*std::cos(angle);
    return std::isfinite(x)&&std::isfinite(y);
}

// Native fallback for authored/loaded tree sprites whose original linked
// obst_base(345) was absent. Also guards an event-generation regression:
// the original invisible 15x17 trunk exists irrespective of foliage art.
inline bool source_standing_tree_trunk_contains(
    const WorldPlacement& tree,float x,float y) noexcept {
    float bx{},by{};
    if(!source_standing_tree_trunk_position(tree,bx,by))return false;
    return source_solid_sprite_contains(
        bx,by,world_source::kOriginalTreeHitboxWidth,
        world_source::kOriginalTreeHitboxHeight,
        world_source::kOriginalTreeHitboxOriginX,
        world_source::kOriginalTreeHitboxOriginY,
        tree.angle,x,y,tree.scale_x);
}

// 3DS-only visibility adaptation. The original root tree event tree has no
// per-player tree SetOpacity event: full source Trees layer43 renders over
// player layer32. At 400x240 this hides the survivor. Fade ONLY the
// foreground canopy that actually covers the player's original hotspot;
// do not mutate source sprite state, hitboxes, saves, or offscreen trees.
inline float source_tree_render_depth(float original_depth,
                                      float player_depth,
                                      bool occludes) noexcept {
    // Citro2D depth test still rejects the player if a translucent
    // foreground canopy writes a *nearer* depth first. Temporarily move
    // only the overlapping tree behind the player, preserving the regular
    // original Map43 vs player Map32 order for all other trees.
    return occludes ? std::min(original_depth,
                               std::max(0.0f,player_depth-0.002f))
                    : original_depth;
}

inline bool source_tree_occludes_player(
    const WorldPlacement& tree,float player_x,float player_y) noexcept {
    const auto* point=world_source::source_tree_impact_point(tree.object);
    if(!point||tree.source_frame!=0||tree.opacity<=0.0f||
       !std::isfinite(player_x)||!std::isfinite(player_y))return false;
    const float width=tree.width>0.0f?tree.width:point->width;
    const float height=tree.height>0.0f?tree.height:point->height;
    if(!source_solid_sprite_contains(tree.x,tree.y,width,height,
        point->origin_x,point->origin_y,tree.angle,
        player_x,player_y,tree.scale_x))return false;
    float trunk_x{},trunk_y{};
    if(!source_standing_tree_trunk_position(tree,trunk_x,trunk_y))return false;
    return player_y<=trunk_y+world_source::kOriginalTreeHitboxHeight*0.25f;
}
} // namespace minidayz::terrain
