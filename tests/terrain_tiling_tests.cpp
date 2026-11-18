#include "minidayz/terrain_tiling.hpp"
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <limits>

using minidayz::TerrainTileWindow;
using minidayz::calculate_terrain_tile_window;

int main() {
    TerrainTileWindow visible{};
    // The original tilemap sheets contain 60x60 source Construct cells.
    assert(calculate_terrain_tile_window(0, 0, 400, 240, 60, visible));
    assert(visible.first_column == 0 && visible.last_column == 7);
    assert(visible.first_row == 0 && visible.last_row == 4);
    // No gaps when the camera moves across a tile boundary.
    assert(calculate_terrain_tile_window(149, 149, 400, 240, 60, visible));
    assert(visible.first_column == 2 && visible.last_column == 10);
    assert(visible.first_row == 2 && visible.last_row == 7);
    // Original Map authored strips are around X=480 and X=17007.
    // A fresh spawn can be anywhere in the 36,850-pixel-wide Map.
    for (float x : {0.0f, 1010.0f, 5500.0f, 18000.0f, 35500.0f}) {
        assert(calculate_terrain_tile_window(x, 4600, 400, 240, 60, visible));
        assert(visible.columns() >= 7 && visible.columns() <= 8);
        assert(visible.rows() >= 4 && visible.rows() <= 5);
        const float left = visible.first_column * 60.0f;
        const float right = visible.last_column * 60.0f;
        assert(left <= x && right >= x + 400.0f);
    }
    assert(!calculate_terrain_tile_window(0, 0, 400, 240, 0, visible));
    assert(!calculate_terrain_tile_window(NAN, 0, 400, 240, 60, visible));
    assert(!calculate_terrain_tile_window(0, 0, -400, 240, 60, visible));
    assert(!calculate_terrain_tile_window(0, 0, 400, 240,
          std::numeric_limits<float>::infinity(), visible));
    assert(!calculate_terrain_tile_window(0, 0, 400, 240, 1, visible));
    return 0;
}
