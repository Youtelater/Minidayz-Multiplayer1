#include "minidayz/crafting.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>

using namespace minidayz::items;

int main() {
    assert(kSourceCraftRecipeCount == 9);
    assert(source_craft_recipe(0)->result_id == 163);
    assert(source_craft_recipe(1)->result_id == 454);
    assert(source_craft_recipe(2)->result_id == 455);
    assert(source_craft_recipe(2)->ingredients[1].item_id == 38);
    assert(source_craft_recipe(2)->ingredients[1].count == 3);
    for (std::size_t i = 3; i < 6; ++i) {
        assert(source_craft_recipe(i)->result_id == 23);
        assert(source_craft_recipe(i)->result_in_inventory);
        assert(source_craft_recipe(i)->ingredients[0].item_id == 50);
    }
    assert(source_craft_recipe(3)->ingredients[1].item_id == 26);
    assert(source_craft_recipe(4)->ingredients[1].item_id == 38);
    assert(source_craft_recipe(5)->ingredients[1].item_id == 49);
    assert(!source_craft_recipe(0)->result_in_inventory);
    assert(source_craft_recipe(6)->result_id == 51);
    assert(source_craft_recipe(9) == nullptr);

    InventorySystem player;
    GroundLootSystem ground;
    assert(!can_craft_from_inventory(player, ground, 100, 120, 1));
    assert(!craft_from_inventory(player, ground, 100, 120, 1));
    assert(!craft_from_inventory(player, ground, 100, 120, 999));
    assert(player.occupied_slots() == 0 && ground.count() == 0);

    assert(player.add(41, 1)); // rope
    assert(!craft_from_inventory(player, ground, 100, 120, 1));
    assert(player.count(41) == 1 && ground.count() == 0);
    assert(player.add(40, 1)); // burlap sack
    assert(can_craft_from_inventory(player, ground, 100, 120, 1));
    assert(craft_from_inventory(player, ground, 100, 120, 1));
    assert(player.count(41) == 0 && player.count(40) == 0);
    assert(player.count(454) == 0); // original spawns result on ground
    assert(ground.count() == 1);
    assert(ground.entry(0)->item.item_id == 454);
    assert(ground.entry(0)->x == 100 && ground.entry(0)->y == 120);

    assert(ground.pickup_nearest(player, 100, 120, 1));
    assert(player.count(454) == 1 && ground.count() == 0);

    assert(player.add(38, 2));
    assert(!can_craft_from_inventory(player, ground, 100, 120, 2));
    assert(!craft_from_inventory(player, ground, 100, 120, 2));
    assert(player.count(38) == 2 && player.count(454) == 1);
    assert(player.add(38, 1));
    assert(can_craft_from_inventory(player, ground, 100, 120, 2));
    assert(craft_from_inventory(player, ground, 100, 120, 2));
    assert(player.count(38) == 0 && player.count(454) == 0);
    assert(player.count(455) == 0);
    assert(ground.count() == 1 && ground.entry(0)->item.item_id == 455);

    // Ground output persists with its exact original result ID and location.
    GroundLootSystem persisted_ground;
    assert(persisted_ground.restore(ground.snapshot()));
    assert(persisted_ground.entry(0)->item.item_id == 455);
    assert(persisted_ground.pickup_nearest(player, 100, 120, 1));
    InventorySystem restored;
    assert(restored.restore(player.snapshot()));
    assert(restored.count(455) == 1);

    InventorySystem bow;
    GroundLootSystem bow_ground;
    assert(bow.add(37, 1)); // ashwood stick
    assert(bow.add(41, 1)); // rope
    assert(can_craft_from_inventory(bow, bow_ground, 25, 50, 0));
    assert(craft_from_inventory(bow, bow_ground, 25, 50, 0));
    assert(bow.count(163) == 0);
    assert(bow.count(37) == 0 && bow.count(41) == 0);
    assert(bow_ground.count() == 1);
    assert(bow_ground.entry(0)->item.item_id == 163);

    InventorySystem mixed;
    GroundLootSystem mixed_ground;
    assert(mixed.add_to_slot(2, 38, 2, 0, 90));
    assert(mixed.add_to_slot(3, 38, 1, 0, 80));
    assert(mixed.add_to_slot(7, 454, 1, 0, 55));
    assert(craft_from_inventory(mixed, mixed_ground, 1, 2, 2));
    assert(mixed.slot(2)->empty() && mixed.slot(3)->empty());
    assert(mixed.count(454) == 0);
    assert(mixed_ground.entry(0)->item.item_id == 455);

    // Original Craft_3: one Woodpiles plus one of Rags, Wooden Sticks,
    // Newspapers produces Campfire Kit directly in inventory. Unlike bow
    // crafting, no ground object is created.
    for (std::size_t i = 3; i < 6; ++i) {
        InventorySystem kit_inventory;
        GroundLootSystem kit_ground;
        assert(kit_inventory.add(50, 1));
        const auto tinder = source_craft_recipe(i)->ingredients[1].item_id;
        assert(!craft_from_inventory(kit_inventory, kit_ground, 1, 2, i));
        assert(kit_inventory.add(tinder, 1));
        assert(can_craft_from_inventory(kit_inventory, kit_ground, 1, 2, i));
        assert(craft_from_inventory(kit_inventory, kit_ground, 1, 2, i));
        assert(kit_inventory.count(50) == 0);
        assert(kit_inventory.count(tinder) == 0);
        assert(kit_inventory.count(23) == 1);
        assert(kit_ground.count() == 0);
    }
    // The result must fit after ingredients are consumed, not before.
    InventorySystem tight;
    GroundLootSystem tight_ground;
    assert(tight.add_to_slot(0, 50, 1));
    assert(tight.add_to_slot(1, 26, 1));
    for (std::size_t i = 2; i < tight.capacity(); ++i)
        assert(tight.add_to_slot(i, 42, 1));
    assert(tight.occupied_slots() == tight.capacity());
    assert(craft_from_inventory(tight, tight_ground, 1, 2, 3));
    assert(tight.count(23) == 1 && tight_ground.count() == 0);

    // Arrow crafting uses any of the three original knives as a reusable
    // catalyst. The result is Check_space_inventory(-1,51), not a ground drop.
    for (std::size_t recipe_index = 6; recipe_index < 9; ++recipe_index) {
        InventorySystem arrow_inventory;
        GroundLootSystem arrow_ground;
        const auto* recipe = source_craft_recipe(recipe_index);
        assert(recipe && recipe->result_in_inventory &&
               recipe->preserve_second_ingredient);
        const auto knife_id = recipe->ingredients[1].item_id;
        assert(knife_id == 42 || knife_id == 43 || knife_id == 44);
        assert(arrow_inventory.add(38, 2));
        assert(!can_craft_from_inventory(
            arrow_inventory, arrow_ground, 100, 100, recipe_index));
        assert(arrow_inventory.add(knife_id, 1, 0, 47));
        assert(can_craft_from_inventory(
            arrow_inventory, arrow_ground, 100, 100, recipe_index));
        assert(craft_from_inventory(
            arrow_inventory, arrow_ground, 100, 100, recipe_index));
        assert(arrow_inventory.count(38) == 1);
        assert(arrow_inventory.count(51) == 1);
        assert(arrow_inventory.count(knife_id) == 1);
        assert(arrow_ground.count() == 0);
        bool preserved = false;
        for (std::size_t slot = 0; slot < arrow_inventory.capacity(); ++slot) {
            const auto* item = arrow_inventory.slot(slot);
            if (item && item->item_id == knife_id) {
                assert(item->condition == 47);
                preserved = true;
            }
        }
        assert(preserved);
    }

    // Full original ground-entity container and nonfinite positions reject a
    // craft without changing the source inventory or dropping the output.
    InventorySystem failed;
    GroundLootSystem full_ground;
    assert(failed.add(37, 1));
    assert(failed.add(41, 1));
    for (std::size_t i = 0; i < kMaximumGroundLootItems; ++i)
        assert(full_ground.add({1, 1}, static_cast<float>(i), 0.0f));
    assert(full_ground.count() == kMaximumGroundLootItems);
    assert(!can_craft_from_inventory(failed, full_ground, 5, 5, 0));
    assert(!craft_from_inventory(failed, full_ground, 5, 5, 0));
    assert(failed.count(37) == 1 && failed.count(41) == 1);
    assert(full_ground.count() == kMaximumGroundLootItems);
    GroundLootSystem empty_ground;
    assert(!craft_from_inventory(failed, empty_ground,
                                 std::numeric_limits<float>::quiet_NaN(), 1, 0));
    assert(failed.count(37) == 1 && failed.count(41) == 1);
    assert(empty_ground.count() == 0);
    return 0;
}
