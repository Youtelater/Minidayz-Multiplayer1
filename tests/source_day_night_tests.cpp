#include "minidayz/source_day_night.hpp"
#include "minidayz/source_night_mask.hpp"
#include <array>
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
    using namespace minidayz::world_source;
    assert(source_night_destination_out(255,255,0.5f)==127);
    assert(source_night_destination_out(200,0,1.0f)==200);
    assert(source_night_destination_out(200,255,1.0f)==0);
    // Overlay shader multiplies sampled alpha by the isolated layer alpha
    // before the original blend8 operation discards its RGB.
    assert(source_night_overlay_destination_out(255,255,0.5f)==127);
    assert(source_night_overlay_destination_out(128,255,0.5f)==96);
    assert(source_night_overlay_destination_out(128,255,1.0f)==64);
    assert(source_night_overlay_destination_out(0,255,1.0f)==0);
    assert(source_night_overlay_destination_out(128,0,1.0f)==128);
    std::array<std::uint8_t,16> mask{};
    const std::uint8_t alpha[]{255,0,128,255};
    mask.fill(200);
    source_night_apply_light(mask.data(),4,4,alpha,2,2,1,1,2,2,0,0,0,false,1);
    assert(mask[5]==0 && mask[6]==200 && mask[9]==100 && mask[10]==0);
    mask.fill(200);
    source_night_apply_light(mask.data(),4,4,alpha,2,2,3,1,2,2,0,0,0,true,1);
    assert(mask[5]==200 && mask[6]==0 && mask[9]==0 && mask[10]==100);
    mask.fill(128);
    source_night_apply_light(mask.data(),4,4,alpha,2,2,1,1,2,2,0,0,0,false,0.5f,true);
    assert(mask[5]==96 && mask[6]==128 && mask[9]==112 && mask[10]==96);
    source_night_apply_light(mask.data(),4,4,alpha,2,2,1,1,2,2,0,0,0,false,0.5f,true);
    assert(mask[5]==78 && mask[6]==128 && mask[10]==78);
    assert(source_night_tiled_index(0,0,512)==0);
    assert(source_night_tiled_index(1,0,512)==1);
    assert(source_night_tiled_index(0,1,512)==2);
    assert(source_night_tiled_index(7,7,512)==63);
    assert(source_night_tiled_index(8,0,512)==64);
    assert(source_night_tiled_index(0,8,512)==4096);
    SourceDayNightClock clock;
    SourceTwilightFade twilight;
    // A fresh 06:01 game did not cross the original 05:30 creation event.
    assert(near(twilight.opacity(),0.0f));
    auto advance_twilight=[&](float dt){
        const auto before=clock.snapshot();
        assert(clock.update(dt));
        twilight.advance(before,clock.snapshot(),dt);
    };
    assert(clock.restore({1109,1.0f,true}));
    advance_twilight(0.5f);
    assert(twilight.frame()==0 && near(twilight.opacity(),0.0f));
    advance_twilight(32.5f);
    assert(near(twilight.opacity(),0.5f));
    advance_twilight(32.5f);
    assert(near(twilight.opacity(),1.0f));
    advance_twilight(12.5f);
    assert(near(twilight.opacity(),0.5f));
    advance_twilight(12.5f);
    assert(near(twilight.opacity(),0.0f));
    assert(clock.restore({329,1.0f,false}));
    advance_twilight(1.0f);
    assert(twilight.frame()==1 && near(twilight.opacity(),0.5f/65.0f));
    twilight.reset();
    assert(near(twilight.opacity(),0.0f));
    clock.reset();
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
