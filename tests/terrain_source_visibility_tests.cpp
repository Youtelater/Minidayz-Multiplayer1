#include "minidayz/terrain_source_visibility.hpp"
#include <cassert>
#include <cmath>

int main() {
    using minidayz::terrain::source_sprite_in_view;
    using minidayz::terrain::source_sprite_top_left;
    using minidayz::terrain::source_sprite_draw_origin;
    // Small sprites outside 400x240 must be culled.
    assert(!source_sprite_in_view(800,500,32,32,.5f,.5f,1,1,0,0,0,400,240));
    // A source 1200px building at a far-away hotspot still overlaps view.
    assert(source_sprite_in_view(700,120,1200,300,.5f,.5f,1,1,0,0,0,400,240));
    // Anchor-aware: a top-left origin is not a centered sprite.
    assert(source_sprite_in_view(-10,70,180,90,0,0,1,1,0,0,0,400,240));
    assert(!source_sprite_in_view(-185,70,180,90,0,0,1,1,0,0,0,400,240));
    // The source hotspot must remain stable when flipping the UVs, despite
    // Citro2D using absolute width for geometry with a negative scale.
    assert(source_sprite_top_left(410,180,0,-1)==230.0f);
    assert(source_sprite_top_left(410,180,0,1)==410.0f);
    assert(source_sprite_top_left(410,180,.5f,-1)==320.0f);
    // Preserve reversed scale for original horizontally mirrored sprites.
    assert(source_sprite_in_view(410,70,180,90,0,0,-1,1,0,0,0,400,240));
    // A Construct house/door rotates around its hotspot, not image top-left.
    constexpr float quarter_turn=1.5707963267948966f;
    const auto rotated=source_sprite_draw_origin(
        100,200,20,10,.25f,.5f,1,1,quarter_turn);
    assert(std::fabs(rotated.x-105.0f)<0.001f);
    assert(std::fabs(rotated.y-195.0f)<0.001f);
    // After rotating uncentered Citro2D, the old top-left-to-pivot vector
    // lands exactly on the original Construct hotspot.
    assert(std::fabs(rotated.x-5.0f-100.0f)<0.001f);
    assert(std::fabs(rotated.y+5.0f-200.0f)<0.001f);
    const auto mirrored=source_sprite_draw_origin(
        410,70,180,90,0,0,-1,1,0);
    assert(mirrored.x==230.0f && mirrored.y==70.0f);
    // Culling must use the corrected 90-degree rendered bounding box.
    assert(source_sprite_in_view(100,100,80,10,.5f,.5f,1,1,
        quarter_turn,93,70,14,20));
    assert(!source_sprite_in_view(100,100,80,10,.5f,.5f,1,1,
        quarter_turn,50,145,15,20));
    assert(source_sprite_in_view(390,120,800,24,.5f,.5f,1,1,
        quarter_turn,0,0,400,240));
    assert(!source_sprite_in_view(500,120,800,24,.5f,.5f,1,1,
        quarter_turn,0,0,400,240));
    // Avoid out-of-bounds math for bad source geometry.
    assert(!source_sprite_in_view(NAN,0,32,32,.5f,.5f,1,1,0,0,0,400,240));
    assert(!source_sprite_in_view(0,0,0,32,.5f,.5f,1,1,0,0,0,400,240));
}
