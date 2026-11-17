#include "minidayz/source_day_night.hpp"
#include <cassert>
#include <limits>

using namespace minidayz::items;

int main() {
    SourceDayNightClock clock;
    assert(clock.snapshot().timer_all==361);
    assert(clock.hour()==6 && clock.minute()==1 && clock.day_mode());
    assert(clock.update(1.49f));
    assert(clock.snapshot().timer_all==361);
    assert(clock.update(0.02f));
    assert(clock.snapshot().timer_all==362);
    assert(clock.minute()==2 && clock.day_mode());
    auto before=clock.snapshot();
    assert(!clock.update(-1.0f));
    assert(!clock.update(std::numeric_limits<float>::infinity()));
    assert(clock.snapshot().timer_all==before.timer_all);
    assert(clock.snapshot().elapsed_seconds==before.elapsed_seconds);
    // 23:59 -> 24:00 retains Day_mode, then 24:01 resets to 00:00 night.
    SourceDayNightSnapshot sunset{1439,0.0f,true};
    assert(clock.restore(sunset));
    assert(clock.update(1.5f));
    assert(clock.hour()==24 && clock.minute()==0 && clock.day_mode());
    assert(clock.update(1.5f));
    assert(clock.hour()==0 && clock.minute()==0 && !clock.day_mode());
    assert(clock.update(359.0f));
    assert(clock.hour()==3 && !clock.day_mode());
    // At 06:00 original UID 650766536843728 sets Day_mode to one.
    assert(clock.restore({359,0.0f,false}));
    assert(clock.update(1.5f));
    assert(clock.hour()==6 && clock.minute()==0 && clock.day_mode());
    // Bunker guard suppresses Day_mode writes, not Timer_ALL itself.
    assert(clock.restore({359,0.0f,false}));
    assert(clock.update(1.5f,true));
    assert(clock.hour()==6 && !clock.day_mode());
    assert(!clock.restore({361,0.0f,false}));
    assert(!clock.restore({0,0.0f,true}));
    assert(!clock.restore({2000,0.0f,false}));
    assert(!clock.restore({361,1.5f,true}));
    assert(!clock.restore({361,-1.0f,true}));
    assert(!clock.restore({361,std::numeric_limits<float>::quiet_NaN(),true}));
    clock.reset();
    assert(clock.hour()==6 && clock.minute()==1 && clock.day_mode());
    return 0;
}
