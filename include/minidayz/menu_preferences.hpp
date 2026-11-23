#pragma once
#include <cstdint>

namespace minidayz::menu {
inline constexpr std::uint32_t kMenuPreferencesMagic=0x4d445a4du; // MDZM
inline constexpr std::uint16_t kMenuPreferencesVersion=1;
// Root Start events define Player_skin_type 1..21, including biker744.
// Keep menu.cfg binary v1 compatible while accepting its original last skin.
inline constexpr std::uint8_t kRootOriginalSelectableSkinCount=21;
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
           v.skin_index<kRootOriginalSelectableSkinCount && v.difficulty<3 && v.muted<=1;
}
bool load_menu_preferences(MenuPreferences& prefs) noexcept;
bool save_menu_preferences(const MenuPreferences& prefs) noexcept;
} // namespace minidayz::menu
