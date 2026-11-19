#include "minidayz/generated/menu_source_catalog.hpp"
#include <cassert>
#include <cstring>
int main(){
    using namespace minidayz::menu;
    static_assert(kSourceAchievementCount==33);
    static_assert(kSourceUnlockCount==12);
    for(std::size_t i=0;i<kSourceAchievementCount;++i){
        assert(kSourceAchievements[i].localization_id==101+i);
        assert(kSourceAchievements[i].name[0]!=0);
        assert(kSourceAchievements[i].description[0]!=0);
    }
    for(std::size_t i=0;i<kSourceUnlockCount;++i){
        assert(kSourceUnlocks[i].localization_id==
               (i<9?71+i:140+i-9));
        assert(kSourceUnlocks[i].name[0]!=0);
        assert(std::strstr(kSourceUnlocks[i].description,"Unlock Requirement")!=nullptr);
    }
    static_assert(kSourceWeaponUnlockCount==6);
    constexpr unsigned expected[]={392,393,394,416,418,420};
    for(std::size_t i=0;i<kSourceWeaponUnlockCount;++i){
        assert(kSourceWeaponUnlocks[i].localization_id==expected[i]);
        assert(kSourceWeaponUnlocks[i].name[0]!=0);
        assert(std::strstr(kSourceWeaponUnlocks[i].description,
                           "Unlock Requirement")!=nullptr);
    }
}
