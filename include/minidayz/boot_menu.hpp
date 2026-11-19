#pragma once
#include <cstdint>
#include <cstddef>

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

inline constexpr TouchBounds kContinueButton{18, 58, 302, 106};
inline constexpr TouchBounds kNewGameButton{18, 115, 302, 163};
// The old skin selector stole the lower part of New Game's hitbox.
// Keep every menu action on an independent visible touch row.
inline constexpr TouchBounds kSkinPrevious{18, 174, 155, 207};
inline constexpr TouchBounds kSkinNext{165, 174, 302, 207};

enum class SkinAction : std::int8_t { none, previous, next };
constexpr SkinAction skin_touch_action(int px,int py) noexcept {
    if(kSkinPrevious.contains(px,py))return SkinAction::previous;
    if(kSkinNext.contains(px,py))return SkinAction::next;
    return SkinAction::none;
}
constexpr std::size_t advance_skin(std::size_t current,std::size_t count,
                                    SkinAction action) noexcept {
    if(count==0)return 0;
    current%=count;
    if(action==SkinAction::previous)return (current+count-1)%count;
    if(action==SkinAction::next)return (current+1)%count;
    return current;
}

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
