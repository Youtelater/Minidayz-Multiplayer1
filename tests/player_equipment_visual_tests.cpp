#include "minidayz/player_equipment_visual.hpp"
#include <cassert>
#include <cstddef>

int main(){
    using namespace minidayz::stage1;
    for(std::size_t index=0;index<kOriginalWearableVisualCount;++index){
        const auto& row=kOriginalWearableVisuals[index];
        assert(row.item>0 && row.object>0);
        if(index>0) assert(row.item>kOriginalWearableVisuals[index-1].item);
        assert(original_equipment_visual(row.item)==&row);
        const auto clothing=original_equipment_pose(&row,false,false);
        const auto movement=original_equipment_pose(&row,true,false);
        const auto weapon=original_equipment_pose(&row,false,true);
        // Source extraction records an entire four-direction family for
        // each advertised pose, never a single incomplete direction.
        if(row.animations & 1u)assert(clothing);
        if(row.animations & 2u)assert(movement);
        if(row.animations & ~3u)assert(weapon);
    }
    assert(original_equipment_visual(65535u)==nullptr);
    assert(original_equipment_pose(nullptr,false,false)==nullptr);
}
