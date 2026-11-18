#include "minidayz/source_rain_timer.hpp"
#include <cassert>
#include <limits>

using namespace minidayz::items;

int main() {
    static_assert(SourceRainTimer::kOriginalRainSeconds==360.0f);
    SourceRainTimer rain;
    for (unsigned i=0;i<359;++i)
        assert(rain.update(1.0f,2,false,4));
    assert(!rain.active());
    assert(rain.snapshot().timer_firings==0);
    assert(rain.update(1.0f,2,false,4));
    assert(rain.active());
    assert(rain.snapshot().rain.last_event_uid==609331584746158ULL);
    assert(rain.snapshot().timer_firings==1);

    // Rain active on the next actual timer callback: original Rain=1
    // stops rain, regardless of the new RNG draw.
    for(unsigned i=0;i<360;++i)
        assert(rain.update(1.0f,2,false,1));
    assert(!rain.active());
    assert(rain.snapshot().rain.last_event_uid==448001804847237ULL);

    // choose(1,2,3,4)==1 does NOT start a new storm.
    for(unsigned i=0;i<360;++i)
        assert(rain.update(1.0f,2,false,1));
    assert(!rain.active());
    assert(rain.snapshot().timer_firings==3);

    // The original rains_level==0 disables both start and stop branches,
    // but does not prevent the timer from ticking.
    auto state=rain.snapshot();
    for(unsigned i=0;i<360;++i)
        assert(rain.update(1.0f,0,false,4));
    assert(!rain.active());
    assert(rain.snapshot().timer_firings==4);
    assert(rain.snapshot().rain.last_event_uid==state.rain.last_event_uid);

    // inRaid==1 prevents rain starting, as in source UID 609331584746158.
    for(unsigned i=0;i<360;++i)
        assert(rain.update(1.0f,2,true,4));
    assert(!rain.active());
    for(unsigned i=0;i<360;++i)
        assert(rain.update(1.0f,3,false,2));
    assert(rain.active());

    const auto saved=rain.snapshot();
    SourceRainTimer restored;
    assert(restored.restore(saved));
    assert(restored.active() && restored.snapshot().timer_firings==6);
    auto invalid=saved;
    invalid.elapsed_seconds=360.0f;
    assert(!restored.restore(invalid));
    invalid=saved;
    invalid.rain.last_event_uid=0;
    assert(!restored.restore(invalid));
    assert(restored.active());
    assert(!restored.update(-0.01f,2,false,3));
    assert(!restored.update(
        std::numeric_limits<float>::quiet_NaN(),2,false,3));
    assert(!restored.update(0.5f,2,false,0));
    assert(restored.snapshot().timer_firings==saved.timer_firings);
    return 0;
}
