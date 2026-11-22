#include "minidayz/source_interior_container.hpp"
#include <cassert>

int main() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    WorldPlacement box{};
    box.object=kSourceBunkerLootbox;
    box.layer=kSourceInteriorContainerLayer;
    box.runtime_uid=1;
    box.width=24;
    box.height=16;
    box.opacity=1.0f;
    assert(kSourceBunkerLootboxOpenEvent==389950912936355ULL);
    for(unsigned animation=0;animation<4;++animation) {
        box.source_animation=static_cast<std::uint16_t>(animation);
        box.source_frame=0;
        assert(source_bunker_lootbox_closed(box));
        // Changing to exact original Sprite796 frame1 latches the box.
        box.source_frame=kSourceBunkerLootboxOpenedFrame;
        assert(!source_bunker_lootbox_closed(box));
    }
    box.source_animation=4;
    box.source_frame=0;
    assert(!source_bunker_lootbox_closed(box));
    box.source_animation=0;
    box.opacity=0;
    assert(!source_bunker_lootbox_closed(box));
    box.opacity=1;
    box.layer=41;
    assert(!source_bunker_lootbox_closed(box));
    box.layer=kSourceInteriorContainerLayer;
    box.object=314; // original trash-container obstacle, not a lootbox
    assert(!source_bunker_lootbox_closed(box));
    box.object=kSourceBunkerLootbox;
    box.runtime_uid=0;
    assert(!source_bunker_lootbox_closed(box));
}
