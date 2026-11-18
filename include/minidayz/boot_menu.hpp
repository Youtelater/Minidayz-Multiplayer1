#pragma once
#include <cstdint>

namespace minidayz::menu {

// Coordinates are for the Nintendo 3DS bottom (320x240) touch display.
// Both the renderer and the touch handler use these same original-art slots.
struct TouchBounds {
    int left, top, right, bottom;
    constexpr bool contains(int x, int y) const noexcept {
        return x >= left && x <= right && y >= top && y <= bottom;
    }
    constexpr float x() const noexcept { return static_cast<float>(left); }
    constexpr float y() const noexcept { return static_cast<float>(top); }
    constexpr float width() const noexcept {
        return static_cast<float>(right - left);
    }
    constexpr float height() const noexcept {
        return static_cast<float>(bottom - top);
    }
};

inline constexpr TouchBounds kContinueButton{18, 78, 302, 132};
inline constexpr TouchBounds kNewGameButton{18, 145, 302, 202};

enum class BootAction : std::uint8_t { none, continue_game, new_game };

constexpr BootAction touch_action(int px, int py, bool has_saved_game) noexcept {
    if (kNewGameButton.contains(px, py)) return BootAction::new_game;
    if (has_saved_game && kContinueButton.contains(px, py))
        return BootAction::continue_game;
    return BootAction::none;
}

constexpr BootAction button_action(bool pressed_a, bool pressed_x,
                                    bool has_saved_game) noexcept {
    if (pressed_x) return BootAction::new_game;
    if (pressed_a) return has_saved_game
        ? BootAction::continue_game : BootAction::new_game;
    return BootAction::none;
}

} // namespace minidayz::menu
