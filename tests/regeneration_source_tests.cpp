#include "minidayz/regeneration_source.hpp"
#include <cassert>
#include <limits>

using namespace minidayz;
using namespace minidayz::items;

int main() {
    assert(source_regeneration_per_original_tick(SurvivalDifficulty::novice) == 2.0f);
    assert(source_regeneration_per_original_tick(SurvivalDifficulty::regular) == 1.75f);
    assert(source_regeneration_per_original_tick(SurvivalDifficulty::veteran) == 1.5f);
    SourceRegenerationConditions guards{};
    guards.stat14 = guards.stat13 = guards.stat16 = guards.stat50 = 55.0f;
    guards.source_status52_inactive = true;
    guards.source_perk_shengbing_inactive = true;
    guards.original_five_second_event = true;
    assert(source_regeneration_conditions_met(guards));
    PlayerState state{};
    state.health=40.0f;
    state.hunger=100.0f;
    state.thirst=100.0f;
    PlayerSystem player(state);
    SourcePerkSystem perks;
    assert(apply_source_regeneration_event(player, guards, SurvivalDifficulty::regular, perks));
    assert(player.state().health == 41.75f);
    assert(perks.award_experience(100u));
    assert(perks.buy(SourcePerk::regen));
    assert(apply_source_regeneration_event(player, guards, SurvivalDifficulty::regular, perks));
    assert(player.state().health == 45.25f);
    assert(apply_source_regeneration_event(player, guards, SurvivalDifficulty::regular, perks));
    assert(player.state().health == 48.75f);
    assert(apply_source_regeneration_event(player, guards, SurvivalDifficulty::regular, perks));
    assert(player.state().health == 52.25f);
    // Original bonus branch is disabled at health >= 50.
    assert(apply_source_regeneration_event(player, guards, SurvivalDifficulty::regular, perks));
    assert(player.state().health == 54.0f);
    const float unchanged = player.state().health;
    guards.original_five_second_event = false;
    assert(!apply_source_regeneration_event(player, guards, SurvivalDifficulty::regular, perks));
    assert(player.state().health == unchanged);
    guards.original_five_second_event = true;
    guards.stat16 = 50.0f;
    assert(!apply_source_regeneration_event(player, guards, SurvivalDifficulty::regular, perks));
    guards.stat16 = std::numeric_limits<float>::quiet_NaN();
    assert(!source_regeneration_conditions_met(guards));
    guards.stat16 = 51.0f;
    guards.stat22 = 1.0f;
    assert(!source_regeneration_conditions_met(guards));
    guards.stat22 = 0.0f;
    guards.source_status52_inactive = false;
    assert(!source_regeneration_conditions_met(guards));
    guards.source_status52_inactive = true;
    guards.source_perk_shengbing_inactive = false;
    assert(!source_regeneration_conditions_met(guards));
    return 0;
}
