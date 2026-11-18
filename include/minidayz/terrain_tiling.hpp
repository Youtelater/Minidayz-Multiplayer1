#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

namespace minidayz {

// Number of ORIGINAL source terrain tiles intersecting the visible 3DS
// camera rectangle. The renderer never allocates the whole 36k x 20k map.
struct TerrainTileWindow {
    int first_column{}, last_column{};
    int first_row{}, last_row{};
    constexpr int columns() const noexcept { return last_column - first_column; }
    constexpr int rows() const noexcept { return last_row - first_row; }
};

inline bool calculate_terrain_tile_window(
    float camera_x, float camera_y, float view_width, float view_height,
    float tile_pixels, TerrainTileWindow& out) noexcept {
    if (!std::isfinite(camera_x) || !std::isfinite(camera_y) ||
        !std::isfinite(view_width) || !std::isfinite(view_height) ||
        !std::isfinite(tile_pixels) || tile_pixels < 1.0f ||
        view_width <= 0.0f || view_height <= 0.0f) return false;
    const double inv = 1.0 / static_cast<double>(tile_pixels);
    const double x0 = std::floor(static_cast<double>(camera_x) * inv);
    const double y0 = std::floor(static_cast<double>(camera_y) * inv);
    const double x1 = std::ceil((static_cast<double>(camera_x) + view_width) * inv);
    const double y1 = std::ceil((static_cast<double>(camera_y) + view_height) * inv);
    if (x0 < std::numeric_limits<int>::min() ||
        y0 < std::numeric_limits<int>::min() ||
        x1 > std::numeric_limits<int>::max() ||
        y1 > std::numeric_limits<int>::max() ||
        x1 <= x0 || y1 <= y0) return false;
    const double n = (x1-x0)*(y1-y0);
    if (n > 2048) return false; // prevent pathological draw-call floods
    out = {static_cast<int>(x0), static_cast<int>(x1),
           static_cast<int>(y0), static_cast<int>(y1)};
    return true;
}

} // namespace minidayz
