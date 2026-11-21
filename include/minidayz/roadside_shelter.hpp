#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz {

struct WorldRectangle {
    float x, y, width, height;

    bool contains(float px, float py) const noexcept {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

struct RoadsideShelter {
    static constexpr float half_width = 30.0f;
    static constexpr float half_height = 22.0f;
    static constexpr std::size_t maximum_count = 3;

    float x{}, y{};
    bool beacon{};
    std::uint16_t supply_item{};

    WorldRectangle floor() const noexcept {
        return {x - half_width, y - half_height, 2 * half_width, 2 * half_height};
    }

    std::array<WorldRectangle, 3> walls() const noexcept {
        const auto area = floor();
        // The south side remains open, including the approach to supplies.
        return {{{area.x, area.y, area.width, 4.0f},
                 {area.x, area.y + 4.0f, 4.0f, area.height - 4.0f},
                 {area.x + area.width - 4.0f, area.y + 4.0f, 4.0f, area.height - 4.0f}}};
    }

    float supply_y() const noexcept { return y + 10.0f; }

    bool blocks(float px, float py) const noexcept {
        for (const auto& wall : walls())
            if (wall.contains(px, py)) return true;
        return false;
    }
};

} // namespace minidayz
