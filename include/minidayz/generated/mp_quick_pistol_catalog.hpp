#pragma once
#include <cstdint>
namespace minidayz::items {
// Generated from root mp_kit.js handgun category; mp_players.js quick slot.
inline constexpr std::uint16_t kMpQuickPistolItemIds[]={151,152,153,165,170,171,172,176,180,181,182,234,235,236,239,244};
inline constexpr bool is_mp_quick_pistol(std::uint16_t item) noexcept {
    for(auto id:kMpQuickPistolItemIds)if(id==item)return true;
    return false;
}
} // namespace minidayz::items
