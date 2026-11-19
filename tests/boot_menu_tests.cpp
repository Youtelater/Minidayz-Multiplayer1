#include "minidayz/boot_menu.hpp"
#include <cassert>

int main() {
    using namespace minidayz::menu;
    static_assert(kContinueButton.contains(18,58));
    static_assert(kContinueButton.contains(302,106));
    static_assert(kNewGameButton.contains(18,115));
    static_assert(kNewGameButton.contains(302,163));
    static_assert(kSkinPrevious.contains(18,174));
    static_assert(kSkinNext.contains(302,207));
    // Every button on the lower screen is independently clickable.
    static_assert(!kNewGameButton.contains(20,174));
    static_assert(!kSkinPrevious.contains(160,145));
    static_assert(!kSkinNext.contains(160,145));
    assert(touch_action(160,80,false)==BootAction::none);
    assert(touch_action(160,80,true)==BootAction::continue_game);
    assert(touch_action(160,138,false)==BootAction::new_game);
    assert(touch_action(160,138,true)==BootAction::new_game);
    assert(touch_action(160,156,false)==BootAction::new_game);
    assert(touch_action(160,190,false)==BootAction::none);
    assert(touch_action(3,3,true)==BootAction::none);
    assert(touch_action(303,80,true)==BootAction::none);
    assert(skin_touch_action(40,190)==SkinAction::previous);
    assert(skin_touch_action(260,190)==SkinAction::next);
    assert(skin_touch_action(160,190)==SkinAction::none);
    assert(skin_touch_action(160,140)==SkinAction::none);
    assert(advance_skin(0,20,SkinAction::previous)==19);
    assert(advance_skin(19,20,SkinAction::next)==0);
    assert(advance_skin(8,20,SkinAction::none)==8);
    assert(advance_skin(8,0,SkinAction::next)==0);
    // An emulator's startup A/X press must NOT skip the boot menu.
    StartInputGate gate{};
    for(int frame=0;frame<25;++frame)
        assert(!gate.update(0.05f,true));
    assert(!gate.armed);
    // The neutral frame arms the menu but never activates a button itself.
    assert(!gate.update(0.05f,false));
    assert(gate.armed);
    assert(gate.update(0.05f,false));
    gate.reset();
    for(int frame=0;frame<40;++frame)
        assert(!gate.update(0.05f,true));
    assert(!gate.armed);
    assert(!gate.update(0.05f,false));
    assert(gate.armed);
    assert(gate.update(0.05f,true));
    assert(button_action(false,false,false)==BootAction::none);
    assert(button_action(true,false,false)==BootAction::new_game);
    assert(button_action(true,false,true)==BootAction::continue_game);
    assert(button_action(false,true,true)==BootAction::new_game);
    assert(button_action(true,true,true)==BootAction::new_game);
}
