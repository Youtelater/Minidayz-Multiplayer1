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

    // Original source mechanic values only; the individual event's
    // conditions are checked separately by its native gameplay caller.
    SourcePerkSystem effects;
    assert(effects.source_melee_defense_percent_bonus() == 0);
    assert(effects.source_melee_critical_bonus() == 0);
    assert(effects.source_trunk_bonus() == 0);
    assert(effects.source_bleeding_timer_seconds() == 60);
    assert(effects.source_sweethome_needs_gain() == 0.0f);
    assert(effects.source_runner_speed_bonus() == 0);
    assert(effects.source_farmer_growth_bonus() == 0);
    assert(effects.award_experience(50000));
    assert(effects.buy(SourcePerk::meleedef));
    assert(effects.source_melee_defense_percent_bonus() == 5);
    assert(effects.buy(SourcePerk::meleedef));
    assert(effects.source_melee_defense_percent_bonus() == 15);
    assert(effects.buy(SourcePerk::meleeatk));
    assert(effects.source_melee_critical_bonus() == 10);
    assert(effects.buy(SourcePerk::trunkster));
    assert(effects.source_trunk_bonus() == 10);
    assert(effects.buy(SourcePerk::metabolism));
    assert(effects.source_bleeding_timer_seconds() == 60);
    assert(effects.buy(SourcePerk::metabolism));
    assert(effects.source_bleeding_timer_seconds() == 30);
    assert(effects.buy(SourcePerk::sweethome));
    assert(effects.source_sweethome_needs_gain() == 0.0f);
    assert(effects.buy(SourcePerk::sweethome));
    assert(effects.source_sweethome_needs_gain() == 0.15f);
    assert(effects.buy(SourcePerk::runner));
    assert(effects.source_runner_speed_bonus() == 5);
    assert(effects.buy(SourcePerk::runner));
    assert(effects.source_runner_speed_bonus() == 15);
    assert(effects.buy(SourcePerk::farmer));
    assert(effects.source_farmer_growth_bonus() == 10);

    // The Construct source exposes sixteen two-level perks. All can be
    // purchased through the exact cumulative XP threshold progression.
    assert(kOriginalPerkCount == 16);
    assert(!p.can_buy(static_cast<SourcePerk>(255)));
    assert(!p.buy(static_cast<SourcePerk>(255)));
    assert(kOriginalPerkMenuOrder[3] == SourcePerk::camel);
    assert(kOriginalPerkMenuOrder[4] == SourcePerk::hamster);
    assert(kOriginalPerkMenuOrder[7] == SourcePerk::regen);
    assert(p.award_experience(100000));
    for (std::size_t i = 0; i < kOriginalPerkCount; ++i) {
        const auto perk = kOriginalPerkMenuOrder[i];
        while (p.level(perk) < 2) {
            assert(p.can_buy(perk));
            assert(p.buy(perk));
        }
        assert(!p.can_buy(perk));
    }
    assert(p.snapshot().perks_learnt_count == 32);
    assert(p.snapshot().other_levels[static_cast<unsigned>(SourcePerk::regen) - 2] == 2);
    assert(!p.can_buy(SourcePerk::regen));

    SourcePerkSystem restored;
    assert(restored.restore(p.snapshot()));
    assert(restored.level(SourcePerk::camel) == 2);
    const auto previous = restored.snapshot();
    auto corrupt = previous;
    corrupt.hamster_level = 3;
    assert(!restored.restore(corrupt));
    corrupt = previous;
    corrupt.other_levels[static_cast<unsigned>(SourcePerk::regen) - 2] = 3;
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
    assert(!restored.award_original_xp_event(0u));
    assert(!restored.award_original_xp_event(851818489505241ULL)); // source SET, not ADD
    assert(restored.award_original_xp_event(662365640635533ULL));
    assert(restored.snapshot().experience_points == 25u);
    assert(restored.award_original_xp_event(265440217696674ULL));
    assert(restored.snapshot().experience_points == 325u);
    restored.clear();
    assert(!restored.award_experience(0));
    assert(restored.award_experience(std::numeric_limits<std::uint32_t>::max()));
    assert(!restored.award_experience(1));
    assert(restored.snapshot().experience_points ==
           std::numeric_limits<std::uint32_t>::max());
    return 0;
}
