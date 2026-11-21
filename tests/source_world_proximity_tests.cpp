#include "minidayz/source_world_proximity.hpp"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using minidayz::world_source::source_sprite_might_touch_player;
    // The old fixed 96px door gate incorrectly rejects an original door
    // whose hotspot is far from its far-side collision area.
    assert(source_sprite_might_touch_player(
        1000.0f,1000.0f,450.0f,40.0f,1.0f,1190.0f,1000.0f));
    // An original wide interior (or roof) extends beyond the old 256px gate.
    assert(source_sprite_might_touch_player(
        1000.0f,1000.0f,900.0f,600.0f,1.0f,1450.0f,1000.0f));
    // Rotation around an off-center Construct image hotspot is covered.
    assert(source_sprite_might_touch_player(
        100.0f,100.0f,180.0f,180.0f,1.0f,220.0f,220.0f));
    // Reflected/negative-scale original sprites cannot be wrongly culled.
    assert(source_sprite_might_touch_player(
        1000.0f,1000.0f,150.0f,50.0f,-2.0f,1200.0f,1000.0f));
    // Far-away objects must still be rejected for efficient 3DS updates.
    assert(!source_sprite_might_touch_player(
        0.0f,0.0f,32.0f,32.0f,1.0f,600.0f,600.0f));
    // Faded roofs MUST be rechecked even after the player has left.
    assert(source_sprite_might_touch_player(
        0.0f,0.0f,32.0f,32.0f,1.0f,600.0f,600.0f,true));
    // A failed input must not propagate NaN into a gameplay update.
    assert(!source_sprite_might_touch_player(
        0.0f,0.0f,32.0f,32.0f,1.0f,
        std::numeric_limits<float>::quiet_NaN(),0.0f));
    assert(!source_sprite_might_touch_player(
        0.0f,0.0f,0.0f,32.0f,1.0f,0.0f,0.0f));
    return 0;
}
