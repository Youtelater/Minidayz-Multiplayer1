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
    WorldPlacement solid{};
    solid.object=world_source::kOriginalTreeHitboxObject;
    solid.source_frame=world_source::kOriginalTreeStandingHitboxFrame;
    solid.x=bx;solid.y=by;
    solid.width=world_source::kOriginalTreeHitboxWidth;
    solid.height=world_source::kOriginalTreeHitboxHeight;
    solid.origin_x=world_source::kOriginalTreeHitboxOriginX;
    solid.origin_y=world_source::kOriginalTreeHitboxOriginY;
    solid.angle=tree.angle;solid.scale_x=tree.scale_x;
    return source_world_solid_mask_contains(solid,x,y);
}

// Source Sprite210 tree_block has its own 348 Solid for each standing
// frame. Preserve the authored collider footprint for orphaned/static
// trees rather than making the entire canopy solid.
inline bool source_tree_block_solid_contains(
    const WorldPlacement& tree,float x,float y) noexcept {
    if(tree.object!=210||tree.opacity<=0.0f)return false;
    const auto* variant=world_source::source_tree_block_frame(tree.source_frame);
    if(!variant)return false;
    const float width=tree.width>0.0f?tree.width:variant->width;
    const float height=tree.height>0.0f?tree.height:variant->height;
    constexpr float radians=0.01745329251994329577f;
    const float angle=tree.angle*radians;
    const float dx=(variant->point_x-variant->origin_x)*width*
                    (tree.scale_x<0.0f?-1.0f:1.0f);
    const float dy=(variant->point_y-variant->origin_y)*height;
    const float bx=tree.x+dx*std::cos(angle)-dy*std::sin(angle);
    const float by=tree.y+dx*std::sin(angle)+dy*std::cos(angle);
    WorldPlacement solid{};
    solid.object=348;solid.source_frame=variant->solid_frame;
    solid.x=bx;solid.y=by;solid.width=variant->solid_width;
    solid.height=variant->solid_height;
    solid.origin_x=variant->solid_origin_x;solid.origin_y=variant->solid_origin_y;
    solid.angle=tree.angle;solid.scale_x=tree.scale_x;
    return source_world_solid_mask_contains(solid,x,y);
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
    if(tree.opacity<=0.0f||!std::isfinite(player_x)||
       !std::isfinite(player_y))return false;
    const auto* point=world_source::source_tree_impact_point(tree.object);
    const auto* block=tree.object==210
        ? world_source::source_tree_block_frame(tree.source_frame) : nullptr;
    if((!point||tree.source_frame!=0)&&!block)return false;
    const float width=tree.width>0.0f?tree.width:
                      (block?block->width:point->width);
    const float height=tree.height>0.0f?tree.height:
                       (block?block->height:point->height);
    const float origin_x=block?block->origin_x:point->origin_x;
    const float origin_y=block?block->origin_y:point->origin_y;
    if(!source_solid_sprite_contains(tree.x,tree.y,width,height,
        origin_x,origin_y,tree.angle,player_x,player_y,tree.scale_x))
        return false;
    if(block){
        const float mid_y=tree.y+
            (block->point_y-block->origin_y)*height;
        return player_y<=mid_y+block->solid_height*0.25f;
    }
    float trunk_x{},trunk_y{};
    if(!source_standing_tree_trunk_position(tree,trunk_x,trunk_y))
        return false;
    return player_y<=trunk_y+world_source::kOriginalTreeHitboxHeight*0.25f;
}
} // namespace minidayz::terrain
