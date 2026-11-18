#pragma once
#include "minidayz/generated/player_equipment_visuals.hpp"
#include <algorithm>
#include <cstdint>

namespace minidayz::stage1 {

// The bit positions and order are extracted from original Construct animation
// names in build_player_equipment_visuals.py, never guessed sprite frame IDs.
inline constexpr const char* kEquipmentPosePrefixes[]={
    "idle","run","pistol","axe","twohand","rifle","shotgun","idle","run"
};

inline const OriginalWearableVisual* original_equipment_visual(
    std::uint16_t item) noexcept {
    const auto* begin=kOriginalWearableVisuals;
    const auto* end=begin+kOriginalWearableVisualCount;
    const auto* found=std::lower_bound(begin,end,item,
        [](const OriginalWearableVisual& row,std::uint16_t wanted){
            return row.item<wanted;
        });
    return found!=end && found->item==item ? found : nullptr;
}

// Clothing follows the character's exact original idle/run direction. A held
// weapon prefers the original weapon stance, then falls back only to poses
// that genuinely exist in the selected original Construct object.
inline const char* original_equipment_pose(
    const OriginalWearableVisual* visual,bool moving,bool weapon) noexcept {
    if(!visual)return nullptr;
    constexpr unsigned idle=1u,run=2u;
    if(weapon){
        // Carry the original held weapon while running; switch to an
        // original pistol/axe/two-hand stance when standing still.
        if(moving && (visual->animations & run)) return "run";
        for(unsigned bit=2;bit<=6;++bit)
            if(visual->animations & (1u<<bit))
                return kEquipmentPosePrefixes[bit];
        if(visual->animations & idle)return "idle";
        if(visual->animations & run)return "run";
        return nullptr;
    }
    if(moving && (visual->animations & run)) return "run";
    if(visual->animations & idle) return "idle";
    if(visual->animations & run) return "run";
    return nullptr;
}

} // namespace minidayz::stage1
