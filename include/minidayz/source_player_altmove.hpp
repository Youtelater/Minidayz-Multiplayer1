#pragma once
#include <algorithm>
#include <cstdint>

namespace minidayz::stage1 {
// Root Game_events -> Animations, Set AltMove MaxSpeed action
// UID 156884066447061 on source PlayerBase object 193:
// 100 - instance_variable6 - instance_variable7
//     + instance_variable4 + instance_variable3
//     + instance_variable5 + instance_variable8.
// A game-input "run" toggle only selects the run_* animated poses. Source
// speed modifiers are event-driven, not an invented run/walk multiplier.
struct SourcePlayerAltMoveModifiers {
    float damage_slowdown{};  // PlayerBase var6: tank hit, wire contact
    float terrain_slowdown{}; // PlayerBase var7: original tile checks
    float energy_boost{};     // PlayerBase var4: Edrink timer
    float runner_bonus{};     // PlayerBase var3: runner perk +5/+15
    float regen_bonus{};      // PlayerBase var5: source perk_regen +10
    float equipment_bonus{};  // PlayerBase var8: source equipment events
};
// Root Game_events / Player stats UID 189013257553857:
 // System Compare perk_regen > 1 (Construct comparator 4) sets
 // PlayerBase193 instance variable5 to +10. At level 1 this speed
 // boost MUST remain zero even though other regeneration effects may run.
inline float source_player_regen_speed_bonus(
    std::uint8_t source_regen_perk_level) noexcept {
    return source_regen_perk_level > 1 ? 10.0f : 0.0f;
}
inline float source_player_altmove_max_speed(
    const SourcePlayerAltMoveModifiers& m) noexcept {
    return std::max(0.0f,100.0f-m.damage_slowdown-m.terrain_slowdown+
                    m.energy_boost+m.runner_bonus+m.regen_bonus+
                    m.equipment_bonus);
}
} // namespace minidayz::stage1
