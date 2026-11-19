#include "minidayz/generated/menu_source_catalog.hpp"
#include <cassert>
#include <cstring>
int main(){
    using namespace minidayz::menu;
    static_assert(kSourceAchievementCount==33);
    static_assert(kSourceUnlockCount==9);
    for(std::size_t i=0;i<kSourceAchievementCount;++i){
        assert(kSourceAchievements[i].localization_id==101+i);
        assert(kSourceAchievements[i].name[0]!=0);
        assert(kSourceAchievements[i].description[0]!=0);
    }
    for(std::size_t i=0;i<kSourceUnlockCount;++i){
        assert(kSourceUnlocks[i].localization_id==71+i);
        assert(kSourceUnlocks[i].name[0]!=0);
        assert(std::strstr(kSourceUnlocks[i].description,"Unlock Requirement")!=nullptr);
    }
}
