#pragma once
#include "minidayz/world.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/original_initial_building_loot.hpp"
#include "minidayz/generated/original_car_trunk_loot.hpp"
#include "minidayz/generated/source_house_companions.hpp"
#include <cmath>
#include <cstdint>
#include <random>
#include <algorithm>

namespace minidayz::items {

// Source Game_events car trunk: object241, search once, draw random(100)
// against 50+trunk_bonus, and choose from an authored frame-specific pool.
// Trunkster level 2 may create one additional identical-pool item with
// independent random(100)<10. No generic car/house loot substitutions.
enum class OriginalCarSearchResult : std::uint8_t {
    invalid, empty, found, capacity_failure
};

// Source Game_events UID1074032691412307 / UID3302144065259174
// requires family1041 overlaps Sprite241 before its loot/XP child.
// Family1041 is the original 17-car family, never a guessed object ID.
// The source spawn event also links the search marker to its car body.
inline bool original_car_trunk_has_source_body(
    const WorldSystem& world,const WorldPlacement& marker) noexcept {
    if(marker.object!=241 || marker.opacity<=0.0f)return false;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* body=world.placement(i);
        if(!body || body->opacity<=0.0f ||
           !stage1::source_family_contains(1041,body->object))continue;
        if(world_source::source_house_sprites_overlap(*body,marker))
            return true;
    }
    return false;
}

inline const OriginalCarTrunkPool* original_car_pool(
    std::uint32_t frame) noexcept {
    if(frame>5)return nullptr;
    for(const auto& p:kOriginalCarTrunkPools)
        if(p.frame_mask & (std::uint8_t{1}<<frame))return &p;
    return nullptr;
}

inline bool original_car_trunk_chance(std::uint32_t draw,
                                     std::uint8_t bonus=0) noexcept {
    // Construct random(100) covers [0,100), with a strict '<' check.
    const double sampled=static_cast<double>(draw)*
        (100.0/4294967296.0);
    return sampled < kOriginalCarTrunkBaseChance+bonus;
}
inline bool original_car_trunk_extra_chance(std::uint32_t draw) noexcept {
    const double sampled=static_cast<double>(draw)*
        (100.0/4294967296.0);
    return sampled < kOriginalCarTrunkExtraChance;
}

inline std::uint16_t original_car_trunk_item(
    const OriginalCarTrunkPool& pool,std::uint32_t draw) noexcept {
    if(!pool.count)return 0;
    const auto offset=static_cast<std::size_t>(
        (static_cast<std::uint64_t>(draw)*pool.count)>>32);
    return source_initial_item_after_default_unlocks(
        kOriginalCarTrunkItems[pool.first+offset]);
}

inline OriginalCarSearchResult source_search_original_car_trunk(
    WorldSystem& world,std::size_t index,GroundLootSystem& ground,
    std::uint8_t trunk_bonus,std::uint8_t perk_trunkster_level,
    std::mt19937& rng) noexcept {
    const auto* car=world.placement(index);
    if(!car || car->object!=241 || !car->runtime_uid ||
       car->source_variable_0!=0 || perk_trunkster_level>2 ||
       trunk_bonus!=(perk_trunkster_level>0
           ?kOriginalCarTrunkBonusOnTrunkster:0))
        return OriginalCarSearchResult::invalid;
    // Source family1041 Overlaps Sprite241 is a real inherited guard.
    // A matching car sprite filename or marker alone grants no loot.
    if(!original_car_trunk_has_source_body(world,*car))
        return OriginalCarSearchResult::invalid;
    const auto* pool=original_car_pool(car->source_frame);
    if(!pool)return OriginalCarSearchResult::invalid;
    const auto* point=world_source::source_house_image_point(
        car->object,car->source_animation,car->source_frame,"",
        kOriginalCarTrunkDropImagepoint);
    if(!point)return OriginalCarSearchResult::invalid;
    constexpr float radians=0.01745329251994329577f;
    const float angle=car->angle*radians;
    const float dx=(point->x-point->origin_x)*car->width*
                   (car->scale_x<0.0f?-1.0f:1.0f);
    const float dy=(point->y-point->origin_y)*car->height;
    const float drop_x=car->x+dx*std::cos(angle)-dy*std::sin(angle);
    const float drop_y=car->y+dx*std::sin(angle)+dy*std::cos(angle);
    if(!std::isfinite(drop_x)||!std::isfinite(drop_y))
        return OriginalCarSearchResult::invalid;

    // Consume source RNG only when the same original event is fired.
    // The original order is trunk_rng, choose item if success, then the
    // optional independent level2 extra-roll and its weighted choice.
    std::uint16_t items[2]{};
    unsigned count=0;
    if(original_car_trunk_chance(rng(),trunk_bonus)) {
        const auto chosen=original_car_trunk_item(*pool,rng());
        if(!chosen || !source_item_exists(chosen))
            return OriginalCarSearchResult::invalid;
        items[count++]=chosen;
        if(perk_trunkster_level==2 &&
           original_car_trunk_extra_chance(rng())) {
            const auto bonus_item=original_car_trunk_item(*pool,rng());
            if(!bonus_item || !source_item_exists(bonus_item))
                return OriginalCarSearchResult::invalid;
            items[count++]=bonus_item;
        }
    }
    if(ground.count()+count>kMaximumGroundLootItems)
        return OriginalCarSearchResult::capacity_failure;
    GroundLootSystem after=ground;
    for(unsigned i=0;i<count;++i)
        if(!after.add({items[i],1,kUseSourceItemCondition},
                      std::floor(drop_x+0.5f),std::floor(drop_y+0.5f)))
            return OriginalCarSearchResult::capacity_failure;
    if(!world.mark_source_car_trunk_searched(index))
        return OriginalCarSearchResult::invalid;
    ground=after;
    return count?OriginalCarSearchResult::found:
                 OriginalCarSearchResult::empty;
}

} // namespace minidayz::items
