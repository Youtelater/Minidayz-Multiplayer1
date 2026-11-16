#include "minidayz/contact_damage.hpp"
#include <cassert>
#include <limits>

using namespace minidayz;
using namespace minidayz::items;

int main() {
    PlayerState base{};
    base.x = 100.0f;
    base.y = 200.0f;
    base.health = 100.0f;
    base.hunger = 100.0f;
    base.thirst = 100.0f;
    PlayerSystem player(base);
    InventorySystem inventory;
    EquipmentSystem equipment;
    // Original helmet 251 has exact source flat head protection 4.
    assert(inventory.add(251, 1));
    assert(equipment.equip_from_inventory(inventory, 0, EquipmentSlot::head));
    assert(equipment.protection().helmet == 4.0f);

    NpcActor npc{};
    npc.x = 104.0f;
    npc.y = 200.0f;
    assert(!apply_verified_npc_contact_damage(
        npc, player, equipment, 10.0f, 3.0f));
    assert(player.state().health == 100.0f);
    assert(apply_verified_npc_contact_damage(
        npc, player, equipment, 10.0f, 4.0f));
    assert(player.state().health == 94.0f);
    assert(apply_verified_npc_contact_damage(
        npc, player, equipment, 2.0f, 5.0f));
    assert(player.state().health == 94.0f); // flat armor absorbs the hit

    npc.alive = false;
    assert(!apply_verified_npc_contact_damage(
        npc, player, equipment, 30.0f, 5.0f));
    npc.alive = true;
    assert(!apply_verified_npc_contact_damage(
        npc, player, equipment, 10.0f,
        std::numeric_limits<float>::quiet_NaN()));
    assert(!apply_verified_npc_contact_damage(
        npc, player, equipment,
        std::numeric_limits<float>::infinity(), 5.0f));
    assert(!apply_verified_npc_contact_damage(
        npc, player, equipment, 10.0f, -1.0f));
    assert(player.state().health == 94.0f);

    equipment.apply_damage(player, 200.0f);
    assert(!player.state().alive);
    assert(!apply_verified_npc_contact_damage(
        npc, player, equipment, 10.0f, 5.0f));
    return 0;
}
