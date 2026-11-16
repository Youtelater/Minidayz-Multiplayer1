#include "minidayz/perks.hpp"
#include <cassert>
#include <limits>

using namespace minidayz::items;
int main() {
    SourcePerkSystem p;
    assert(p.snapshot().perk_cost == 100 && p.snapshot().experience_points == 0);
    assert(p.level(SourcePerk::camel) == 0 && p.level(SourcePerk::hamster) == 0);
    assert(!p.item_use_flags().camel_active && !p.item_use_flags().hamster_active);
    assert(!p.can_buy(SourcePerk::hamster));
    assert(!p.buy(SourcePerk::camel));
    assert(p.award_experience(100));
    assert(p.can_buy(SourcePerk::camel));
    assert(p.buy(SourcePerk::camel));
    assert(p.level(SourcePerk::camel) == 1);
    assert(!p.item_use_flags().camel_active);
    assert(p.snapshot().perk_cost == 220); // 100 + 100 + 20*1
    assert(p.snapshot().experience_points == 100);
    assert(!p.can_buy(SourcePerk::camel));
    assert(p.award_experience(120));
    assert(p.buy(SourcePerk::camel));
    assert(p.level(SourcePerk::camel) == 2);
    assert(p.item_use_flags().camel_active && !p.item_use_flags().hamster_active);
    assert(!p.buy(SourcePerk::camel));
    assert(p.snapshot().perk_cost == 360);
    assert(p.award_experience(140));
    assert(p.buy(SourcePerk::hamster));
    assert(p.snapshot().perk_cost == 520);
    assert(p.level(SourcePerk::hamster) == 1);
    assert(!p.item_use_flags().hamster_active);
    assert(p.award_experience(160));
    assert(p.buy(SourcePerk::hamster));
    assert(p.level(SourcePerk::hamster) == 2);
    assert(p.item_use_flags().hamster_active);
    assert(p.snapshot().perk_cost == 700);
    assert(p.snapshot().perks_learnt_count == 4);
    assert(!p.can_buy(SourcePerk::hamster));

    SourcePerkSystem restored;
    assert(restored.restore(p.snapshot()));
    assert(restored.level(SourcePerk::camel) == 2);
    const auto previous = restored.snapshot();
    auto corrupt = previous;
    corrupt.hamster_level = 3;
    assert(!restored.restore(corrupt));
    corrupt = previous;
    corrupt.perk_cost = 0;
    assert(!restored.restore(corrupt));
    corrupt = previous;
    corrupt.perks_learnt_count = 0;
    assert(!restored.restore(corrupt));
    assert(restored.snapshot().perk_cost == previous.perk_cost);
    restored.clear();
    assert(restored.snapshot().perk_cost == 100);
    assert(restored.snapshot().perks_learnt_count == 0);
    assert(restored.level(SourcePerk::camel) == 0);
    assert(restored.level(SourcePerk::hamster) == 0);
    assert(!restored.can_buy(SourcePerk::camel));
    assert(!restored.award_experience(0));
    assert(restored.award_experience(std::numeric_limits<std::uint32_t>::max()));
    assert(!restored.award_experience(1));
    assert(restored.snapshot().experience_points ==
           std::numeric_limits<std::uint32_t>::max());
    return 0;
}
