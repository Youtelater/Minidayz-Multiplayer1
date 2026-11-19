#pragma once
#include <cstdint>

namespace minidayz::menu {
inline constexpr std::uint32_t kMenuPreferencesMagic=0x4d445a4du; // MDZM
inline constexpr std::uint16_t kMenuPreferencesVersion=1;
struct MenuPreferences {
    std::uint32_t magic{kMenuPreferencesMagic};
    std::uint16_t version{kMenuPreferencesVersion};
    std::uint8_t skin_index{};
    std::uint8_t difficulty{1};
    std::uint8_t muted{};
    std::uint8_t reserved[3]{};
};
static_assert(sizeof(MenuPreferences)==12,"Menu config on-disk layout changed");
constexpr bool valid_preferences(const MenuPreferences& v) noexcept {
    return v.magic==kMenuPreferencesMagic &&
           v.version==kMenuPreferencesVersion &&
           v.skin_index<20 && v.difficulty<3 && v.muted<=1;
}
bool load_menu_preferences(MenuPreferences& prefs) noexcept;
bool save_menu_preferences(const MenuPreferences& prefs) noexcept;
} // namespace minidayz::menu
