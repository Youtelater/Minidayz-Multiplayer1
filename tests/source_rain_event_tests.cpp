#include "minidayz/source_rain_event.hpp"
#include <cassert>

using namespace minidayz::items;

int main() {
    SourceRainEventState rain;
    assert(!rain.active);
    assert(rain.last_event_uid == 0);
    assert(!rain.dispatch(609331584746158ULL,false,true));
    assert(!rain.dispatch(609331584746158ULL,true,false));
    assert(!rain.dispatch(448001804847237ULL,true,true));
    assert(!rain.dispatch(0,true,true));
    assert(rain.dispatch(609331584746158ULL,true,true));
    assert(rain.active);
    assert(rain.last_event_uid == 609331584746158ULL);
    assert(!rain.dispatch(609331584746158ULL,true,true));
    assert(!rain.dispatch(448001804847237ULL,true,false));
    assert(rain.active);
    assert(rain.dispatch(448001804847237ULL,true,true));
    assert(!rain.active);
    assert(rain.last_event_uid == 448001804847237ULL);
    assert(!rain.dispatch(448001804847237ULL,true,true));
    return 0;
}
