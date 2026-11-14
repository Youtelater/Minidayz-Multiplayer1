#include "minidayz/item_use.hpp"

#include <cassert>

using namespace minidayz;
using namespace minidayz::items;

int main() {
    ItemUseEffect effect{};
    assert(source_item_use_effect(1, false, effect));
    assert(effect.hunger == 30.0f && effect.thirst == 5.0f);
    assert(source_item_use_effect(8, false, effect));
    assert(effect.thirst == 30.0f);
    assert(source_item_use_effect(19, false, effect));
    assert(effect.health == 50.0f);
    assert(source_item_use_effect(60, true, effect));
    assert(effect.hunger == 25.0f && effect.thirst == 10.0f);
    assert(!source_item_use_effect(456, false, effect));

    PlayerState initial{};
    initial.health = 50.0f;
    initial.hunger = 10.0f;
    initial.thirst = 20.0f;
    initial.temperature = 0.0f;
    initial.alive = true;
    PlayerSystem player(initial);
    PlayerRules rules{};
    rules.max_health = 100.0f;
    rules.max_hunger = 100.0f;
    rules.max_thirst = 100.0f;
    rules.min_temperature = -10.0f;
    rules.max_temperature = 10.0f;

    InventorySystem inventory;
    assert(inventory.add(1, 1));
    assert(use_item_from_inventory(inventory, 0, player, rules));
    assert(inventory.count(1) == 0);
    assert(player.state().hunger == 40.0f);
    assert(player.state().thirst == 25.0f);

    assert(inventory.add(19, 1));
    assert(use_item_from_inventory(inventory, 0, player, rules));
    assert(player.state().health == 100.0f);

    assert(inventory.add(60, 1));
    assert(!use_item_from_inventory(inventory, 0, player, rules, false));
    assert(inventory.count(60) == 1);
    assert(use_item_from_inventory(inventory, 0, player, rules, true));
    assert(player.state().hunger == 65.0f);
    assert(player.state().thirst == 35.0f);
    assert(inventory.count(60) == 0);

    // A source effect with temperature change is clamped by the configured
    // PlayerRules bounds rather than allowing invalid player state.
    assert(inventory.add(25, 1));
    assert(use_item_from_inventory(inventory, 0, player, rules));
    assert(player.state().temperature == 10.0f);
    return 0;
}
