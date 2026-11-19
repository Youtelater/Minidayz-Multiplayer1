#include "minidayz/generated/item_sprite_catalog.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

int main(){
    using namespace minidayz::items;
    // Explicitly retain references from all 3 supplied mp_itemtable.js
    // objects. These are optional multiplayer visuals, NOT source
    // single-player drop/spawn rules.
    static_assert(kItemSpriteDefinitionCount>0);
    static_assert(kMpPlayerDropSpriteCount>0);
    static_assert(kMpSpriteVariantCount>0);
    for(std::size_t i=1;i<kMpPlayerDropSpriteCount;++i)
        assert(kMpPlayerDropSpriteDefinitions[i-1].item_id <
               kMpPlayerDropSpriteDefinitions[i].item_id);
    for(std::size_t i=0;i<kMpPlayerDropSpriteCount;++i)
        assert(kMpPlayerDropSpriteDefinitions[i].object < 65535);
    for(std::size_t i=0;i<kMpSpriteVariantCount;++i){
        assert(kMpSpriteVariants[i].object < 65535);
        assert(kMpSpriteVariants[i].base_object < 65535);
    }
}
