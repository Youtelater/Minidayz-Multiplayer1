#include "minidayz/fractional_wear.hpp"
#include <cassert>

using namespace minidayz::items;

int main() {
    SourceFractionalWear wear;
    assert(wear.snapshot().condition_hundredths == 10000);
    assert(!wear.initialize(101));
    assert(wear.initialize(100));
    for (unsigned i = 0; i < 4; ++i)
        assert(wear.wear(20)); // original 0.2 source events
    assert(wear.snapshot().condition_hundredths == 9920);
    assert(wear.compatible_condition() == 100);
    assert(wear.wear(20));
    assert(wear.exact_condition() == 99.0f);
    assert(wear.compatible_condition() == 99);

    SourceFractionalWear quarters;
    assert(quarters.initialize(100));
    for (unsigned i = 0; i < 4; ++i)
        assert(quarters.wear(25)); // original 0.25 wear
    assert(quarters.exact_condition() == 99.0f);

    SourceFractionalWear other;
    for (unsigned i = 0; i < 5; ++i)
        assert(other.wear(40)); // original 0.4 wear
    assert(other.exact_condition() == 98.0f);
    assert(other.initialize(100));
    assert(other.wear(50)); // original 0.5 wear
    assert(other.wear(50));
    assert(other.exact_condition() == 99.0f);

    const auto saved = other.snapshot();
    SourceFractionalWear loaded;
    assert(loaded.restore(saved));
    assert(loaded.snapshot().condition_hundredths == 9900);
    assert(!loaded.restore({10001}));
    assert(loaded.snapshot().condition_hundredths == 9900);
    assert(loaded.wear(9900));
    assert(loaded.broken());
    assert(loaded.compatible_condition() == 0);
    assert(!loaded.wear(1));
    assert(!loaded.wear(0));
    assert(wear.wear(65535));
    assert(wear.broken());
    return 0;
}
