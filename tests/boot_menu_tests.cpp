#include "minidayz/boot_menu.hpp"
#include <cassert>

int main() {
    using namespace minidayz::menu;
    static_assert(kContinueButton.contains(18, 78));
    static_assert(kContinueButton.contains(302, 132));
    static_assert(kNewGameButton.contains(18, 145));
    static_assert(kNewGameButton.contains(302, 202));
    static_assert(!kNewGameButton.contains(18, 132));

    // The boot screen appears with and without a valid saved game.
    // Without a saved game Continue remains disabled.
    assert(touch_action(160, 101, false) == BootAction::none);
    assert(touch_action(160, 101, true) == BootAction::continue_game);
    assert(touch_action(160, 171, false) == BootAction::new_game);
    assert(touch_action(160, 171, true) == BootAction::new_game);
    assert(touch_action(3, 3, true) == BootAction::none);
    assert(touch_action(303, 101, true) == BootAction::none);
    assert(touch_action(160, 140, true) == BootAction::none);

    assert(button_action(false, false, false) == BootAction::none);
    assert(button_action(true, false, false) == BootAction::new_game);
    assert(button_action(true, false, true) == BootAction::continue_game);
    assert(button_action(false, true, true) == BootAction::new_game);
    assert(button_action(true, true, true) == BootAction::new_game);
    return 0;
}
