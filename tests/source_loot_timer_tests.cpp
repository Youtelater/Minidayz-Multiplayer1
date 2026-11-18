#include "minidayz/source_loot_timer.hpp"
#include <cassert>
#include <limits>
using namespace minidayz::items;
int main() {
    static_assert(SourceLootTimer::kOriginalRespawnSeconds==1480.0f);
    static_assert(SourceLootTimer::kSourceCheckLootEvent==2026232979865106ULL);
    SourceLootTimer timer;
    for(unsigned i=0;i<1479;++i)
        assert(!timer.advance(1.0f));
    assert(timer.snapshot().firings==0);
    assert(timer.advance(1.0f));
    assert(timer.snapshot().firings==1);
    assert(timer.snapshot().elapsed_seconds==0.0f);
    for(unsigned i=0;i<739;++i)
        assert(!timer.advance(1.0f));
    auto saved=timer.snapshot();
    SourceLootTimer restored;
    assert(restored.restore(saved));
    for(unsigned i=0;i<740;++i)
        assert(!restored.advance(1.0f));
    assert(restored.advance(1.0f));
    assert(restored.snapshot().firings==2);
    auto invalid=saved;
    invalid.elapsed_seconds=1480.0f;
    assert(!restored.restore(invalid));
    invalid.elapsed_seconds=std::numeric_limits<float>::quiet_NaN();
    assert(!restored.restore(invalid));
    assert(!restored.advance(-1.0f));
    assert(!restored.advance(2.0f));
    assert(restored.snapshot().firings==2);
    return 0;
}
