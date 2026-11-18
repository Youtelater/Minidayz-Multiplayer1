#include "minidayz/original_timed_loot.hpp"
#include "minidayz/source_loot_timer.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
using namespace minidayz::items;

int main() {
    assert(kOriginalTimedLootPoolCount == 2);
    assert(kOriginalTimedLootChoiceCount > 2);
    assert(kOriginalMapLootMarkerCount == 15);
    assert(kOriginalTimedLootPools[0].seed_pubg == 0);
    assert(kOriginalTimedLootPools[1].seed_pubg == 1);
    SourceLootTimer timer;
    OriginalTimedLootMarker marker{1200.2f, 80.7f, 15, false};
    OriginalTimedLootContext context{0, 0, true, false, false, 0};
    GroundLootSystem ground;
    assert(!source_timed_loot_respawn(ground, marker, context, UINT32_MAX, 0));
    for (int i = 0; i < 1479; ++i) assert(!timer.advance(1.0f));
    assert(timer.advance(1.0f));
    context.check_loot_timer_fired = true;
    assert(!source_timed_loot_respawn(ground, {900.0f, 0.0f, 15, false},
                                    context, UINT32_MAX, 0));
    assert(!source_timed_loot_respawn(ground, marker, context, 0, 0));
    assert(ground.count() == 0);
    assert(source_timed_loot_respawn(ground, marker, context, UINT32_MAX, 0));
    assert(ground.count() == 1);
    assert(ground.entry(0)->x == 1200.0f && ground.entry(0)->y == 81.0f);
    assert(ground.entry(0)->item.item_id ==
           kOriginalTimedLootChoices[kOriginalTimedLootPools[0].first]);
    context.seed_pubg = 1;
    assert(source_timed_loot_respawn(ground, marker, context, 0, UINT32_MAX));
    const auto& pool = kOriginalTimedLootPools[1];
    assert(ground.entry(1)->item.item_id ==
        kOriginalTimedLootChoices[pool.first + pool.count - 1]);
    assert(!source_timed_loot_respawn(ground, {1200,0,14,false},
                                    context, UINT32_MAX, 0));
    context.on_bridge=true;
    assert(!source_timed_loot_respawn(ground, marker, context, UINT32_MAX, 0));
    context.on_bridge=false;
    context.on_map_layout=false;
    assert(!source_timed_loot_respawn(ground, marker, context, UINT32_MAX, 0));
    context.on_map_layout=true;
    context.seed_pubg=0;
    marker.belongs_to_source_family_995=true;
    assert(!source_timed_loot_respawn(ground, marker, context, UINT32_MAX, 0));
    context.seed_pubg=2;
    marker.belongs_to_source_family_995=false;
    assert(!source_timed_loot_respawn(ground, marker, context, UINT32_MAX, 0));
    context.seed_pubg=1;
    assert(!source_timed_loot_respawn(ground,
        {std::numeric_limits<float>::quiet_NaN(),80,15,false},
        context,UINT32_MAX,0));
    assert(ground.count() == 2);

    std::uint16_t category = 0;
    for (const auto& original : kOriginalMapLootMarkers) {
        assert(source_authored_map_loot_marker(original.source_uid, category));
        assert(category == original.category);
    }
    assert(!source_authored_map_loot_marker(UINT64_MAX, category));
    return 0;
}
