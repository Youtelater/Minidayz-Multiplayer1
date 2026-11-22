#pragma once
#include "minidayz/world.hpp"
#include "minidayz/inventory.hpp"
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

// Verified root Craft_bed/Craft_bed1 -> Building_mode(12):
// woodpiles item50 + deerskin714 / bear hide784. The source spawns bed951,
// NOT a third invented backpack item. The ingredient mutation is atomic.
inline constexpr std::uint16_t kSourceBedWoodpiles=50;
inline constexpr std::uint16_t kSourceBedDeerskin=714;
inline constexpr std::uint16_t kSourceBedBearHide=784;
inline constexpr float kSourceBedBuildDelay=3.0f;
inline constexpr float kSourceBedDismantleDelay=1.0f;
inline constexpr float kSourceBedPlaceRadius=100.0f;
inline constexpr std::uint16_t kSourceBedGhostObject=573;
inline constexpr std::uint32_t kSourceBedGhostFrame=12;
inline constexpr float kSourceBedGhostWidth=37.0f;
inline constexpr float kSourceBedGhostHeight=21.0f;
inline constexpr float kSourceBedGhostOriginX=0.6f;
inline constexpr float kSourceBedGhostOriginY=0.7f;
inline constexpr std::size_t kSourceMaxBeds=256;

inline std::uint16_t source_bed_hides(std::uint8_t bear_pi) noexcept {
    return bear_pi==0 ? kSourceBedDeerskin:
        bear_pi==1 ? kSourceBedBearHide:std::uint16_t{0};
}
inline bool source_bed_materials_available(
    const items::InventorySystem& inventory,std::uint8_t bear_pi) noexcept {
    const auto hide=source_bed_hides(bear_pi);
    return hide && inventory.count(kSourceBedWoodpiles)>=1 &&
           inventory.count(hide)>=1;
}
inline bool source_bed_consume_materials(
    items::InventorySystem& inventory,std::uint8_t bear_pi) noexcept {
    if(!source_bed_materials_available(inventory,bear_pi))return false;
    auto changed=inventory;
    for(const auto item_id:{kSourceBedWoodpiles,source_bed_hides(bear_pi)}){
        bool removed=false;
        for(std::size_t i=0;i<changed.capacity();++i){
            const auto* slot=changed.slot(i);
            if(slot && !slot->empty() && slot->item_id==item_id &&
               changed.remove(i,1)){removed=true;break;}
        }
        if(!removed)return false;
    }
    inventory=changed;
    return true;
}
inline bool source_bed_site_within_range(float player_x,float player_y,
                                          float ghost_x,float ghost_y) noexcept {
    return std::isfinite(player_x)&&std::isfinite(player_y) &&
        std::isfinite(ghost_x)&&std::isfinite(ghost_y) &&
        std::hypot(player_x-ghost_x,player_y-ghost_y)<kSourceBedPlaceRadius;
}
} // namespace minidayz::world_source
