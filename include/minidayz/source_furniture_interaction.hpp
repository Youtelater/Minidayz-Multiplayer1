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
// Verified original Bed_activated -> SleepApproved branches from root
// Construct source UIDs 373077181447985, 648183463842982,
// 993930551645035 and 413036232407346. These are ONLY early rejection
// checks. A successful precheck does not authorize sleeping: the original
// also checks raid, shelter/perks, illness and nearby threats, then runs
// Count_sleep_hours and GotoSleep. Never advance time without those events.
enum class SourceBedSleepPrecheck : std::uint8_t {
    passed_known_conditions, daytime, cold, starving, dehydrated,
    invalid
};
inline SourceBedSleepPrecheck source_bed_sleep_precheck(
    std::uint16_t timer_all, float player_temperature,
    float hunger, float thirst) noexcept {
    if(timer_all>1440 || !std::isfinite(player_temperature) ||
       !std::isfinite(hunger) || !std::isfinite(thirst))
        return SourceBedSleepPrecheck::invalid;
    if(timer_all>360 && timer_all<1440)
        return SourceBedSleepPrecheck::daytime;
    if(player_temperature<0.0f)
        return SourceBedSleepPrecheck::cold;
    if(hunger<10.0f)
        return SourceBedSleepPrecheck::starving;
    if(thirst<10.0f)
        return SourceBedSleepPrecheck::dehydrated;
    return SourceBedSleepPrecheck::passed_known_conditions;
}
inline const char* source_bed_precheck_feedback(
    SourceBedSleepPrecheck result) noexcept {
    switch(result) {
    case SourceBedSleepPrecheck::daytime:return "SLEEP RESTRICTED: DAYTIME";
    case SourceBedSleepPrecheck::cold:return "SLEEP RESTRICTED: TOO COLD";
    case SourceBedSleepPrecheck::starving:return "SLEEP RESTRICTED: HUNGRY";
    case SourceBedSleepPrecheck::dehydrated:return "SLEEP RESTRICTED: THIRSTY";
    case SourceBedSleepPrecheck::invalid:return "SLEEP CONDITIONS INVALID";
    default:return "BED FOUND - SLEEP SYSTEM PENDING";
    }
}
// Original game events 903835777805920 resets bed951 to source
// frame0; 948724703580895 selects source frame1 for player181 overlap.
inline std::uint32_t source_bed_overlap_frame(
    const WorldPlacement& bed,bool player_overlaps) noexcept {
    return bed.object==kFurnitureBed && source_furniture_active(bed) &&
           player_overlaps ? 1u : 0u;
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
