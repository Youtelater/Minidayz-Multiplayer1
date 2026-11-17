#include "minidayz/source_wear.hpp"

#include <cassert>
#include <cstddef>

using namespace minidayz::items;

int main() {
    // All action identifiers and exact hundredths are from root Construct
    // group/event/action UIDs, not inferred from a weapon name.
    static_assert(kSourceWearActionCount >= 90);
    static_assert(kSourceWearUntranslatedCount >= 5);
    const auto* firearm = source_wear_action(
        6904772320561696ULL, 875054325568454ULL, 744011060201717ULL);
    assert(firearm && firearm->loss_hundredths == 25);
    assert(firearm->cancel_when_wxnj_active);
    const auto* melee = source_wear_action(
        3528035038609865ULL,2098275646157213ULL,622291358919957ULL);
    assert(melee && melee->loss_hundredths == 25);
    const auto* damage = source_wear_action(
        811368909310732ULL,251602875520018ULL,228602990115248ULL);
    assert(damage && damage->loss_hundredths == 100);

    SourceFractionalWear held;
    assert(held.initialize(100));
    assert(!apply_verified_source_wear(
        6904772320561696ULL,875054325568454ULL,744011060201717ULL,
        false,false,held));
    assert(held.snapshot().condition_hundredths==10000);
    assert(!apply_verified_source_wear(
        6904772320561696ULL,875054325568454ULL,744011060201717ULL,
        true,true,held));
    assert(held.snapshot().condition_hundredths==10000);
    for(unsigned i=0;i<4;++i)
        assert(apply_verified_source_wear(
            6904772320561696ULL,875054325568454ULL,744011060201717ULL,
            true,false,held));
    assert(held.snapshot().condition_hundredths==9900);

    // Same action under a wrong group or wrong action UID must never fire.
    assert(!apply_verified_source_wear(
        5836740205504893ULL,875054325568454ULL,744011060201717ULL,
        true,false,held));
    assert(!apply_verified_source_wear(
        6904772320561696ULL,875054325568454ULL,744011060201718ULL,
        true,false,held));
    assert(held.snapshot().condition_hundredths==9900);

    assert(apply_verified_source_wear(
        811368909310732ULL,251602875520018ULL,228602990115248ULL,
        true,false,held));
    assert(held.snapshot().condition_hundredths==9800);
    const auto saved=held.snapshot();
    SourceFractionalWear restored;
    assert(restored.restore(saved));
    assert(restored.snapshot().condition_hundredths==9800);
    // Unknown nonliteral/dynamic original actions stay unsupported.
    assert(source_wear_action(0,0,0)==nullptr);
    return 0;
}
