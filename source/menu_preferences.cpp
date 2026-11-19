#include "minidayz/menu_preferences.hpp"
#include <cstdio>
#include <sys/stat.h>

namespace minidayz::menu {
namespace {
constexpr const char* kSettingsPath="sdmc:/3ds/Minidayz/menu.cfg";
constexpr const char* kSettingsTemp="sdmc:/3ds/Minidayz/menu.tmp";
}
bool load_menu_preferences(MenuPreferences& prefs) noexcept {
    std::FILE* f=std::fopen(kSettingsPath,"rb");
    if(!f)return false;
    MenuPreferences candidate{};
    const bool ok=std::fread(&candidate,sizeof(candidate),1,f)==1 &&
        std::fgetc(f)==EOF;
    std::fclose(f);
    if(!ok||!valid_preferences(candidate))return false;
    prefs=candidate;
    return true;
}
bool save_menu_preferences(const MenuPreferences& prefs) noexcept {
    if(!valid_preferences(prefs))return false;
    (void)mkdir("sdmc:/3ds",0777);
    (void)mkdir("sdmc:/3ds/Minidayz",0777);
    std::FILE* f=std::fopen(kSettingsTemp,"wb");
    if(!f)return false;
    const bool ok=std::fwrite(&prefs,sizeof(prefs),1,f)==1 &&
        std::fflush(f)==0;
    const bool closed=std::fclose(f)==0;
    if(!ok||!closed){
        (void)std::remove(kSettingsTemp);
        return false;
    }
    if(std::rename(kSettingsTemp,kSettingsPath)!=0){
        (void)std::remove(kSettingsTemp);
        return false;
    }
    return true;
}
} // namespace minidayz::menu
