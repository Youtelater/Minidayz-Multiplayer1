#include "minidayz/menu_preferences.hpp"
#include <cassert>
int main(){
    using namespace minidayz::menu;
    static_assert(sizeof(MenuPreferences)==12);
    constexpr MenuPreferences defaults{};
    static_assert(valid_preferences(defaults));
    MenuPreferences m{};
    m.skin_index=19;
    m.difficulty=2;
    m.muted=1;
    assert(valid_preferences(m));
    m.skin_index=20; // Original biker Sprite744 is the 21st selection.
    assert(valid_preferences(m));
    m.skin_index=21;
    assert(!valid_preferences(m));
    m.skin_index=0;m.difficulty=3;
    assert(!valid_preferences(m));
    m.difficulty=1;m.muted=2;
    assert(!valid_preferences(m));
    m.muted=0;m.version=8;
    assert(!valid_preferences(m));
    m.version=kMenuPreferencesVersion;m.magic=0;
    assert(!valid_preferences(m));
}
