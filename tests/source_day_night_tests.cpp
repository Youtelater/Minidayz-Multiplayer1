#include "minidayz/source_day_night.hpp"
#include <cassert>
#include <limits>

using namespace minidayz::items;

int main() {
    const auto near=[](float x,float y){return std::fabs(x-y)<0.0001f;};
    assert(near(source_outdoor_night_opacity(19,30),0.30f));
    assert(near(source_outdoor_night_opacity(20,0),0.60f));
    assert(near(source_outdoor_night_opacity(24,0),0.60f));
    assert(near(source_outdoor_night_opacity(0,30),0.795f));
    assert(near(source_outdoor_night_opacity(1,0),1.0f));
    assert(near(source_outdoor_night_opacity(4,59),1.0f));
    assert(near(source_outdoor_night_opacity(5,30),0.507f));
    assert(near(source_outdoor_night_opacity(5,59),0.0169f));
    assert(near(source_outdoor_night_opacity(6,0),0.0f));
    assert(near(source_outdoor_night_opacity(18,59),0.0f));
    assert(near(source_outdoor_night_opacity(20,0,2),0.58f));
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
    assert(clock.restore({361,0.0f,false})); // bunker can keep night mode
    assert(clock.restore({0,0.0f,true})); // bunker can keep day mode
    assert(!clock.restore({2000,0.0f,false}));
    assert(!clock.restore({361,1.5f,true}));
    assert(!clock.restore({361,-1.0f,true}));
    assert(!clock.restore({361,std::numeric_limits<float>::quiet_NaN(),true}));
    clock.reset();
    assert(clock.hour()==6 && clock.minute()==1 && clock.day_mode());
    return 0;
}
