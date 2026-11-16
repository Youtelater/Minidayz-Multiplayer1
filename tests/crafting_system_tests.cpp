#include "minidayz/crafting.hpp"

#include <cassert>
#include <cstddef>

using namespace minidayz::items;

int main() {
    // The original game tutorial explicitly documents these three recipes.
    assert(kSourceCraftRecipeCount == 3);
    assert(source_craft_recipe(0)->result_id == 163);
    assert(source_craft_recipe(1)->result_id == 454);
    assert(source_craft_recipe(2)->result_id == 455);
    assert(source_craft_recipe(2)->ingredients[1].item_id == 38);
    assert(source_craft_recipe(2)->ingredients[1].count == 3);
    assert(source_craft_recipe(3) == nullptr);

    InventorySystem player;
    assert(!can_craft_from_inventory(player, 1));
    assert(!craft_from_inventory(player, 1));
    assert(!craft_from_inventory(player, 999));
    assert(player.occupied_slots() == 0);

    assert(player.add(41, 1)); // rope
    assert(!craft_from_inventory(player, 1));
    assert(player.count(41) == 1); // no partial consumption
    assert(player.add(40, 1)); // burlap sack
    assert(can_craft_from_inventory(player, 1));
    assert(craft_from_inventory(player, 1));
    assert(player.count(41) == 0);
    assert(player.count(40) == 0);
    assert(player.count(454) == 1);

    assert(player.add(38, 2));
    assert(!can_craft_from_inventory(player, 2));
    assert(!craft_from_inventory(player, 2));
    assert(player.count(38) == 2);
    assert(player.count(454) == 1);
    assert(player.add(38, 1));
    assert(can_craft_from_inventory(player, 2));
    assert(craft_from_inventory(player, 2));
    assert(player.count(38) == 0);
    assert(player.count(454) == 0);
    assert(player.count(455) == 1);

    InventorySystem bow;
    assert(bow.add(37, 1)); // ashwood stick
    assert(bow.add(41, 1)); // rope
    assert(can_craft_from_inventory(bow, 0));
    assert(craft_from_inventory(bow, 0));
    assert(bow.count(163) == 1);
    assert(bow.count(37) == 0 && bow.count(41) == 0);

    // Distinct original condition values may occupy distinct stacks. Crafting
    // consumes across slots, preserving unrelated items and remaining wear.
    InventorySystem mixed;
    assert(mixed.add_to_slot(2, 38, 2, 0, 90));
    assert(mixed.add_to_slot(3, 38, 1, 0, 80));
    assert(mixed.add_to_slot(7, 454, 1, 0, 55));
    assert(craft_from_inventory(mixed, 2));
    assert(mixed.slot(2)->empty() && mixed.slot(3)->empty());
    assert(mixed.count(455) == 1);
    assert(mixed.count(454) == 0);

    // Persisted inventory round-trips the crafted original item identity.
    InventorySystem restored;
    assert(restored.restore(player.snapshot()));
    assert(restored.count(455) == 1);
    assert(!can_craft_from_inventory(restored, 2));

    return 0;
}
