#include "minidayz/terrain_level.hpp"
#include <cassert>
int main() {
    using minidayz::terrain::active_ground_object;
    using minidayz::terrain::active_ground_texture;
    static_assert(active_ground_object(0) == 482);
    static_assert(active_ground_object(1) == 687);
    static_assert(active_ground_object(2) == 688);
    static_assert(active_ground_object(3) == 776);
    static_assert(active_ground_object(4) == 980);
    static_assert(active_ground_object(5) == 1139);
    static_assert(active_ground_texture(5) == 6);
    static_assert(minidayz::terrain::ground_texture_at(5,10900.0f,11000.0f)==6);
    static_assert(minidayz::terrain::ground_texture_at(5,11000.0f,11000.0f)==5);
    static_assert(minidayz::terrain::ground_texture_at(5,15000.0f,11000.0f)==5);
    static_assert(minidayz::terrain::ground_texture_at(4,15000.0f,11000.0f)==4);
    // Object 1138 has its own original sheet for the later world.
    static_assert(minidayz::terrain::kGroundObjectByTexture[5] == 1138);
    for (unsigned i = 0; i <= 5; ++i) {
        const auto frame = active_ground_texture(static_cast<std::uint8_t>(i));
        assert(frame < 7);
        assert(active_ground_object(static_cast<std::uint8_t>(i)) != 0);
    }
}
