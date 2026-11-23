#pragma once
#include <cstdint>

namespace minidayz::world_source {

// Root Game_events function "wuranqu" UID316200290612230,
// Sprite514.SpawnAnotherObject(1126) on original Map layer44 "Rain",
// followed by Sprite1126.SetPosition(X-500,Y-500). Frame 0 of original
// "duqu-sheet0.png" is 2000x2000 at centered hotspot (0.5,0.5).
inline constexpr std::uint64_t kOriginalRadioactiveFunctionUid=
    316200290612230ULL;
inline constexpr std::uint16_t kOriginalRadioactiveSprite=1126;
inline constexpr std::uint32_t kOriginalRadioactiveFrame=0;
inline constexpr float kOriginalRadioactiveWidth=2000.0f;
inline constexpr float kOriginalRadioactiveHeight=2000.0f;
inline constexpr float kOriginalRadioactiveOriginX=0.5f;
inline constexpr float kOriginalRadioactiveOriginY=0.5f;
inline constexpr float kOriginalRadioactiveOffsetX=-500.0f;
inline constexpr float kOriginalRadioactiveOffsetY=-500.0f;
// First branch: all Mapgen514 type70 source military sites.
inline constexpr std::uint64_t kOriginalRadioactiveMilitarySpawnAction=
    513805253688873ULL;
// Root repeated category branch: selected markers receive var0*1000
// and are restored by child event130235402370103.
inline constexpr std::uint64_t kOriginalRadioactivePickedSpawnAction=
    680790671133170ULL;

constexpr bool original_radioactive_site_type(std::uint16_t type) noexcept {
    switch(type) {
    case 5: case 22: case 102: case 15: case 103:
    case 11: case 6: case 9: case 104: case 106: case 32:
        return true;
    default: return false;
    }
}
constexpr int original_radioactive_site_quota(std::uint8_t level,
                                                bool perk_dao_seven=false) noexcept {
    if(level==3)return 2;
    if(level>=4)return perk_dao_seven?6:3;
    return 0; // Root CurrentLevel==3, or >=4; not levels 0..2.
}
} // namespace minidayz::world_source
