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

// The original Menu_Events has five visible primary choices:
// Continue / New game / Achievements / Unlocks / Options.
// These 320x240 hitboxes are screen adaptations, but share geometry with
// the renderer and physical D-Pad selection.
enum class BootPage : std::uint8_t {
    main, character, achievements, unlocks, options, multiplayer
};
enum class MenuEntry : std::uint8_t {
    continue_game, new_game, achievements, unlocks, options, none
};
inline constexpr TouchBounds kMenuRows[5] = {
    {38, 35, 282, 65},
    {38, 70, 282, 100},
    {38, 105, 282, 135},
    {38, 140, 282, 170},
    {38, 175, 282, 205}
};
inline constexpr TouchBounds kContinueButton=kMenuRows[0];
inline constexpr TouchBounds kNewGameButton=kMenuRows[1];
inline constexpr TouchBounds kAchievementsButton=kMenuRows[2];
inline constexpr TouchBounds kUnlocksButton=kMenuRows[3];
inline constexpr TouchBounds kOptionsButton=kMenuRows[4];
inline constexpr TouchBounds kCharacterButton{13, 210, 152, 239};
inline constexpr TouchBounds kMultiplayerButton{167, 210, 307, 239};
// Dedicated character-select screen: no overlap with New Game.
inline constexpr TouchBounds kSkinPrevious{18, 145, 154, 191};
inline constexpr TouchBounds kSkinNext{166, 145, 302, 191};
inline constexpr TouchBounds kSubpageBack{15, 211, 305, 239};

constexpr MenuEntry main_entry_at(int x,int y) noexcept {
    for(std::size_t i=0;i<5;++i)
        if(kMenuRows[i].contains(x,y))
            return static_cast<MenuEntry>(i);
    return MenuEntry::none;
}
constexpr MenuEntry move_selection(MenuEntry current,int direction,
                                    bool has_save) noexcept {
    int idx=current==MenuEntry::none?0:static_cast<int>(current);
    for(int step=0;step<5;++step){
        idx=(idx+(direction<0?4:1))%5;
        if(has_save||idx!=0)return static_cast<MenuEntry>(idx);
    }
    return MenuEntry::new_game;
}
constexpr MenuEntry default_selection(bool has_save) noexcept {
    return has_save?MenuEntry::continue_game:MenuEntry::new_game;
}
inline constexpr const char* kMainLabels[5]={
    "CONTINUE","NEW GAME","ACHIEVEMENTS","UNLOCKS","OPTIONS"
};

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

// Ignore an emulator launcher/overlay's synthetic A/X or touch press
// immediately after starting the 3DS executable. The menu must remain
// visible until the user releases ALL menu controls and presses again.
struct StartInputGate {
    float elapsed{};
    bool armed{};
    void reset() noexcept { elapsed=0.0f; armed=false; }
    bool update(float dt,bool controls_held) noexcept {
        if(armed)return true;
        // Slow Manic Emu frames can exceed 100ms. The gate must still
        // arm, otherwise the menu will remain frozen at low FPS.
        if(dt>0.0f)elapsed+=(dt>0.10f?0.10f:dt);
        // Require a full second plus one neutral frame on the launcher.
        if(elapsed>=1.0f && !controls_held)armed=true;
        return false;
    }
};

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
