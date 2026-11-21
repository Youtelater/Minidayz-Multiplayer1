#include "minidayz/source_door_auto.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include <cassert>
#include <cmath>

int main() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    WorldPlacement sprite{};
    sprite.x=100.0f;
    sprite.y=200.0f;
    sprite.width=80.0f;
    sprite.height=30.0f;
    sprite.origin_x=0.25f;
    sprite.origin_y=0.5f;
    sprite.scale_x=1.0f;
    sprite.angle=0.0f;
    SourceDoorPoint point{0,0,80.0f,30.0f,0.25f,0.5f,0.8f,0.5f,0.5f,0.666667f,false};

    // At normal orientation the hotspot-relative x range is [80,160].
    assert(source_door_overlaps_player(sprite,point,{145.0f,200.0f}));
    assert(!source_door_overlaps_player(sprite,point,{55.0f,200.0f}));
    assert(source_player_overlaps_world_rect(sprite,{145.0f,200.0f},
        sprite.origin_x,sprite.origin_y));
    assert(!source_player_overlaps_world_rect(sprite,{55.0f,200.0f},
        sprite.origin_x,sprite.origin_y));
    assert(terrain::source_solid_sprite_contains(
        sprite.x,sprite.y,sprite.width,sprite.height,
        sprite.origin_x,sprite.origin_y,sprite.angle,
        145.0f,200.0f,sprite.scale_x));

    // Mirror around the original source hotspot; drawn range [40,120].
    sprite.scale_x=-1.0f;
    assert(!source_door_overlaps_player(sprite,point,{145.0f,200.0f}));
    assert(source_door_overlaps_player(sprite,point,{55.0f,200.0f}));
    assert(!source_player_overlaps_world_rect(sprite,{145.0f,200.0f},
        sprite.origin_x,sprite.origin_y));
    assert(source_player_overlaps_world_rect(sprite,{55.0f,200.0f},
        sprite.origin_x,sprite.origin_y));
    assert(!terrain::source_solid_sprite_contains(
        sprite.x,sprite.y,sprite.width,sprite.height,
        sprite.origin_x,sprite.origin_y,sprite.angle,
        145.0f,200.0f,sprite.scale_x));
    assert(terrain::source_solid_sprite_contains(
        sprite.x,sprite.y,sprite.width,sprite.height,
        sprite.origin_x,sprite.origin_y,sprite.angle,
        55.0f,200.0f,sprite.scale_x));

    // Door and roof ImagePointY includes the rotated, flipped source
    // X-offset. Without this adjustment a mirrored door opens at the
    // opposite side of its visual "in" point.
    constexpr float kTolerance=0.01f;
    sprite.angle=90.0f;
    sprite.scale_x=1.0f;
    const float y_normal=source_door_in_y(sprite,point);
    sprite.scale_x=-1.0f;
    const float y_mirror=source_door_in_y(sprite,point);
    assert(std::fabs(y_normal-244.0f)<kTolerance);
    assert(std::fabs(y_mirror-156.0f)<kTolerance);
    const SourceRoofMemberPoint roof_point{
        321,0,0,80.0f,30.0f,0.25f,0.5f,0.8f,0.5f,0.25f,1.0f};
    assert(std::fabs(source_roof_in_world_y(sprite,roof_point)
                     -156.0f)<kTolerance);
    sprite.scale_x=1.0f;
    assert(std::fabs(source_roof_in_world_y(sprite,roof_point)
                     -244.0f)<kTolerance);
    // A 20x20 square rotated 45 degrees has world extent sqrt(200).
    // At x=32 the old two-axis test accepted a non-touching player.
    sprite.width=20.0f;
    sprite.height=20.0f;
    sprite.origin_x=0.5f;
    sprite.origin_y=0.5f;
    sprite.angle=45.0f;
    assert(!source_player_overlaps_world_rect(sprite,{132.0f,200.0f},0.5f,0.5f));
    assert(source_player_overlaps_world_rect(sprite,{128.0f,200.0f},0.5f,0.5f));
    sprite.scale_x=-1.0f;
    assert(!source_player_overlaps_world_rect(sprite,{132.0f,200.0f},0.5f,0.5f));
    assert(source_player_overlaps_world_rect(sprite,{128.0f,200.0f},0.5f,0.5f));
    return 0;
}
