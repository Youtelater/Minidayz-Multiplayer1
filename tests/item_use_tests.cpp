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
    // Unsupported IDs clear a reused output object rather than leaking the
    // last successful item's values into a caller's next action.
    effect = {99.0f, 98.0f, 97.0f, 96.0f};
    assert(!source_item_use_effect(456, false, effect));
    assert(effect.hunger == 0.0f && effect.thirst == 0.0f &&
           effect.health == 0.0f && effect.temperature == 0.0f);

    // Original item_activate expressions add +10 hunger for perk_hamster
    // and +10 thirst for perk_camel when their source conditions evaluate true.
    ItemUsePerks hamster{};
    hamster.hamster_active = true;
    assert(source_item_use_effect(1, false, hamster, effect));
    assert(effect.hunger == 40.0f && effect.thirst == 5.0f);
    assert(source_item_use_effect(2, false, hamster, effect));
    assert(effect.hunger == 30.0f);
    ItemUsePerks camel{};
    camel.camel_active = true;
    assert(source_item_use_effect(8, false, camel, effect));
    assert(effect.thirst == 40.0f);
    assert(source_item_use_effect(8, false, effect));
    assert(effect.thirst == 30.0f);

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

    // The perk bonus is applied before the same source stat clamp and item consumption.
    PlayerState perk_state{};
    perk_state.health = 100.0f;
    perk_state.hunger = 10.0f;
    perk_state.thirst = 0.0f;
    perk_state.alive = true;
    PlayerSystem perk_player(perk_state);
    InventorySystem perk_inventory;
    assert(perk_inventory.add(1, 1));
    assert(use_item_from_inventory(perk_inventory, 0, perk_player, rules,
                                   false, hamster));
    assert(perk_player.state().hunger == 50.0f);
    assert(perk_player.state().thirst == 5.0f);
    assert(perk_inventory.count(1) == 0);

    // Source item_activate UIDs 9313426168084632/8412413328012037
    // require bleeding status == 1 and reset the actual player field.
    player.set_bleeding(3.0f, 10.0f);
    assert(inventory.add(18, 1, 0, 77));
    assert(use_item_from_inventory(inventory, 0, player, rules));
    assert(player.state().bleeding == 0.0f && inventory.count(18) == 0);
    assert(inventory.add(18, 1));
    assert(!use_item_from_inventory(inventory, 0, player, rules));
    assert(inventory.count(18) == 1);
    assert(inventory.remove(0, 1));
    player.set_bleeding(1.0f, 10.0f);
    assert(inventory.add(26, 1));
    assert(!use_item_from_inventory(inventory, 0, player, rules, true));
    assert(inventory.count(26) == 1);
    assert(use_item_from_inventory(inventory, 0, player, rules));
    assert(player.state().bleeding == 0.0f && inventory.count(26) == 0);

    // Original Tetracycline source action resets player instance var 35.
    player.set_infection(4.0f, 10.0f);
    assert(inventory.add(46, 1));
    assert(use_item_from_inventory(inventory, 0, player, rules));
    assert(inventory.count(46) == 0 && player.state().infection == 0.0f);
    assert(inventory.add(46, 1));
    assert(use_item_from_inventory(inventory, 0, player, rules));
    assert(inventory.count(46) == 0);
    return 0;
}
