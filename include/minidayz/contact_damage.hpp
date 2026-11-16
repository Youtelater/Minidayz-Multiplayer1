#pragma once

#include "minidayz/npc_system.hpp"
#include "minidayz/player.hpp"
#include "minidayz/equipment.hpp"

#include <cmath>

namespace minidayz {

// The original hit event must supply its own attack range, damage amount,
// cooldown and conditions. Do NOT call this per frame: that would invent a
// new combat rule. This is the native damage/armor transaction used once a
// source-verified NPC hit is dispatched.
inline bool apply_verified_npc_contact_damage(
    const NpcActor& attacker, PlayerSystem& player,
    const items::EquipmentSystem& equipment,
    float source_damage, float source_attack_radius) noexcept {
    if (!attacker.alive || !player.state().alive ||
        !std::isfinite(attacker.x) || !std::isfinite(attacker.y) ||
        !std::isfinite(source_damage) || source_damage <= 0.0f ||
        !std::isfinite(source_attack_radius) || source_attack_radius < 0.0f)
        return false;
    const float dx = attacker.x - player.state().x;
    const float dy = attacker.y - player.state().y;
    // Compare distance without taking a square root. Use double so native
    // 3DS world coordinates cannot overflow intermediate square products.
    const double distance_squared = static_cast<double>(dx) * dx +
                                    static_cast<double>(dy) * dy;
    const double range_squared = static_cast<double>(source_attack_radius) *
                                 source_attack_radius;
    if (distance_squared > range_squared)
        return false;
    equipment.apply_damage(player, source_damage);
    return true;
}

} // namespace minidayz
