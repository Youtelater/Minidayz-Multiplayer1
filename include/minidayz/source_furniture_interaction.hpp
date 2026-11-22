#pragma once
#include "minidayz/world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace minidayz::world_source {
// Source object951: constructed bed, original "bed-sheet0.png" frames 0/1.
// Object442: water pump. Both live on original items_on_ground layer18.
// Do not activate the editor's bed951 on Rain layer44.
inline constexpr std::uint16_t kFurnitureBed=951;
inline constexpr std::uint16_t kFurniturePump=442;
inline constexpr std::uint16_t kFurnitureLayer=18;
inline constexpr std::uint64_t kPumpMaxWater=200;
inline constexpr float kPumpUseDelay=2.0f;
inline constexpr float kPumpSip=25.0f;
inline constexpr float kPumpThirstMax=100.0f;

inline bool source_furniture_active(const WorldPlacement& p) noexcept {
    return (p.object==kFurnitureBed || p.object==kFurniturePump) &&
           p.layer==kFurnitureLayer && p.opacity>0.0f &&
           p.width>0.0f && p.height>0.0f && p.runtime_uid!=0;
}
struct SourcePumpDraw {
    std::uint16_t consumed{};
    float thirst_gain{};
    bool can_use{};
};
// Original Construct 2626558775459672 / 5959987082061273:
// 2-second pump interaction, max 25 thirst restored, drain remaining
// water (or all remaining when under 25); cap player thirst at 100.
inline SourcePumpDraw source_pump_draw(float thirst,
                                      std::uint64_t water) noexcept {
    SourcePumpDraw draw{};
    if(!std::isfinite(thirst)||thirst<0.0f||thirst>=kPumpThirstMax||
       water==0||water>kPumpMaxWater)return draw;
    const float gain=std::min<float>(kPumpSip,
        std::min<float>(kPumpThirstMax-thirst,static_cast<float>(water)));
    if(gain<=0.0f)return draw;
    draw.consumed=static_cast<std::uint16_t>(std::ceil(gain));
    draw.thirst_gain=gain;
    draw.can_use=draw.consumed>0;
    return draw;
}
} // namespace minidayz::world_source
