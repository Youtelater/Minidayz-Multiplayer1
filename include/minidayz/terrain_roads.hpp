#pragma once
#include <cmath>
#include <cstdint>

namespace minidayz::terrain {

// Road Array codes from original generate_array_roads_1/2 source events.
constexpr bool horizontal_road(std::uint16_t code) noexcept {
    return code == 13 || code == 14;
}
constexpr bool vertical_road(std::uint16_t code) noexcept {
    return code == 12 || code == 14;
}
struct RoadBand {
    float x{}, y{}, width{}, height{};
    bool valid() const noexcept {
        return std::isfinite(x) && std::isfinite(y) &&
               width > 0.0f && height > 0.0f;
    }
};

// The map-generation Array has one code per world cell. Road corridors
// cross a cell at its centre; use the original 60px source tile width.
inline RoadBand road_band(std::uint16_t code, bool vertical, int cell_x,
                          int cell_y, float cell_width, float cell_height,
                          float road_width) noexcept {
    if ((vertical ? !vertical_road(code) : !horizontal_road(code)) ||
        !(cell_width > 0.0f) || !(cell_height > 0.0f) ||
        !(road_width > 0.0f)) return {};
    const float center_x = (static_cast<float>(cell_x) + 0.5f) * cell_width;
    const float center_y = (static_cast<float>(cell_y) + 0.5f) * cell_height;
    if (vertical)
        return {center_x - road_width * 0.5f,
                static_cast<float>(cell_y) * cell_height,
                road_width, cell_height};
    return {static_cast<float>(cell_x) * cell_width,
            center_y - road_width * 0.5f, cell_width, road_width};
}

inline bool intersects_view(const RoadBand& b, float x, float y,
                            float w, float h) noexcept {
    return b.valid() && b.x < x + w && b.x + b.width > x &&
           b.y < y + h && b.y + b.height > y;
}

} // namespace minidayz::terrain
