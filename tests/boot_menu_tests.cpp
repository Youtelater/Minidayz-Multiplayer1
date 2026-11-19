#include "minidayz/boot_menu.hpp"
#include <cassert>
int main(){
    using namespace minidayz::menu;
    // Five ORIGINAL Menu_Events labels, each with independent touch bounds.
    static_assert(sizeof(kMainLabels)/sizeof(kMainLabels[0])==5);
    static_assert(kContinueButton.contains(60,48));
    static_assert(kNewGameButton.contains(60,80));
    static_assert(kAchievementsButton.contains(60,114));
    static_assert(kUnlocksButton.contains(60,147));
    static_assert(kOptionsButton.contains(60,188));
    static_assert(!kContinueButton.contains(60,81));
    static_assert(!kNewGameButton.contains(60,114));
    static_assert(!kAchievementsButton.contains(60,147));
    static_assert(!kUnlocksButton.contains(60,188));
    static_assert(!kOptionsButton.contains(60,218));
    for(unsigned i=0;i<5;i++){
        assert(main_entry_at(160,(kMenuRows[i].top+kMenuRows[i].bottom)/2)
               ==static_cast<MenuEntry>(i));
        for(unsigned j=0;j<5;j++)
            if(i!=j)
                assert(!kMenuRows[j].contains(160,(kMenuRows[i].top+kMenuRows[i].bottom)/2));
    }
    assert(main_entry_at(15,35)==MenuEntry::none);
    assert(main_entry_at(310,218)==MenuEntry::none);
    assert(kCharacterButton.contains(55,224));
    assert(kMultiplayerButton.contains(230,224));
    assert(!kCharacterButton.contains(230,224));
    assert(!kMultiplayerButton.contains(55,224));
    assert(kSkinPrevious.contains(30,170));
    assert(kSkinNext.contains(270,170));
    assert(!kSkinPrevious.contains(160,170));
    assert(!kSkinNext.contains(160,170));
    assert(kSubpageBack.contains(160,223));
    assert(default_selection(false)==MenuEntry::new_game);
    assert(default_selection(true)==MenuEntry::continue_game);
    assert(move_selection(MenuEntry::continue_game,1,true)==MenuEntry::new_game);
    assert(move_selection(MenuEntry::new_game,-1,true)==MenuEntry::continue_game);
    assert(move_selection(MenuEntry::new_game,-1,false)==MenuEntry::options);
    assert(move_selection(MenuEntry::options,1,false)==MenuEntry::new_game);
    assert(touch_action(160,82,false)==BootAction::new_game);
    assert(touch_action(160,49,false)==BootAction::none);
    assert(touch_action(160,49,true)==BootAction::continue_game);
    assert(skin_touch_action(30,170)==SkinAction::previous);
    assert(skin_touch_action(270,170)==SkinAction::next);
    assert(skin_touch_action(160,170)==SkinAction::none);
    assert(advance_skin(0,20,SkinAction::previous)==19);
    assert(advance_skin(19,20,SkinAction::next)==0);
    assert(advance_skin(8,20,SkinAction::none)==8);
    assert(advance_skin(8,0,SkinAction::next)==0);
    StartInputGate gate{};
    for(int i=0;i<25;++i)assert(!gate.update(0.05f,true));
    assert(!gate.armed);
    assert(!gate.update(0.05f,false));
    assert(gate.armed);
    assert(gate.update(0.05f,true));
    gate.reset();
    for(int i=0;i<40;++i)assert(!gate.update(0.05f,true));
    assert(!gate.armed);
    assert(!gate.update(0.05f,false));
    assert(gate.armed);
}
