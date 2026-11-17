#pragma once

#include "minidayz/player.hpp"
#include "minidayz/perks.hpp"
#include <cmath>
#include <cstdint>

namespace minidayz::items {

// Original Construct Game_events globals:
// Novice_regeneration=2, Regular_regeneration=1.75,
// Veteran_regeneration=1.5; each is assigned to Regeneration_current
// in original Start events UIDs 5179335140440188, 931046879689234
// and 4240905673640528. The event is on an original 5s cadence.
inline float source_regeneration_per_original_tick(
    minidayz::SurvivalDifficulty difficulty) noexcept {
    switch (difficulty) {
    case minidayz::SurvivalDifficulty::novice: return 2.0f;
    case minidayz::SurvivalDifficulty::regular: return 1.75f;
    case minidayz::SurvivalDifficulty::veteran: return 1.5f;
    }
    return 0.0f;
}

// Explicit original Construct Player Stats instance variable guards from
// event 7874329434067746. The native PlayerState does not yet expose
// all these source vars; DO NOT map them to guessed health/temp fields.
// An event dispatcher must populate all fields and confirm the original
// every-5s trigger before this transaction runs.
struct SourceRegenerationConditions {
    float stat14{};
    float stat13{};
    float stat16{};
    float stat50{};
    float stat22{};
    float stat35{};
    bool source_status52_inactive{};
    bool source_perk_shengbing_inactive{};
    bool original_five_second_event{};
};

inline bool source_regeneration_conditions_met(
    const SourceRegenerationConditions& c) noexcept {
    return c.original_five_second_event &&
        std::isfinite(c.stat14) && c.stat14 > 50.0f &&
        std::isfinite(c.stat13) && c.stat13 > 50.0f &&
        std::isfinite(c.stat16) && c.stat16 > 50.0f &&
        std::isfinite(c.stat50) && c.stat50 > 50.0f &&
        std::isfinite(c.stat22) && c.stat22 == 0.0f &&
        std::isfinite(c.stat35) && c.stat35 == 0.0f &&
        c.source_status52_inactive && c.source_perk_shengbing_inactive;
}

inline bool apply_source_regeneration_event(
    minidayz::PlayerSystem& player,
    const SourceRegenerationConditions& conditions,
    minidayz::SurvivalDifficulty difficulty,
    const SourcePerkSystem& perks) noexcept {
    if (!player.state().alive ||
        !source_regeneration_conditions_met(conditions)) return false;
    const float base = source_regeneration_per_original_tick(difficulty);
    if (base <= 0.0f) return false;
    // Original event 3074676324153042 adds one extra
    // Regeneration_current when perk_regen > 0 and health stat 15 < 50.
    const bool regen_bonus = perks.level(SourcePerk::regen) > 0 &&
                              player.state().health < 50.0f;
    player.heal(base * (regen_bonus ? 2.0f : 1.0f), 100.0f);
    return true;
}

} // namespace minidayz::items
