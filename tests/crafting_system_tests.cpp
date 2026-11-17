#include "minidayz/crafting.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>

using namespace minidayz::items;

int main() {
    assert(kSourceCraftRecipeCount == 16);
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
    assert(source_craft_recipe(9)->result_id == 41);
    assert(source_craft_recipe(9)->result_in_inventory);
    assert(source_craft_recipe(9)->selected_stack_only);
    assert(source_craft_recipe(9)->ingredients[0].item_id == 26);
    assert(source_craft_recipe(9)->ingredients[0].count == 3);
    assert(source_craft_recipe(9)->ingredients[1].count == 0);
    assert(source_craft_recipe(10)->result_id == 147);
    assert(source_craft_recipe(11)->result_id == 148);
    assert(source_craft_recipe(12)->result_id == 102);
    assert(source_craft_recipe(13)->result_id == 117);
    assert(!source_craft_recipe(13)->result_in_inventory);
    assert(source_craft_recipe(13)->delay_seconds == 5.0f);
    assert(source_craft_recipe(14)->result_id == 127);
    assert(source_craft_recipe(14)->result_in_inventory);
    assert(source_craft_recipe(14)->delay_seconds == 5.0f);
    assert(source_craft_recipe(15)->result_id == 87);
    assert(!source_craft_recipe(15)->result_in_inventory);
    assert(source_craft_recipe(15)->source_ground_object == 837);
    assert(source_craft_recipe(16) == nullptr);

    // The native ground entry is original Construct t837 (Simple Fishing
    // Rod item 87), created at Player.X/Player.Y + 10 with condition 100.
    InventorySystem fishing;
    GroundLootSystem fishing_ground;
    assert(fishing.add(37, 1)); // Ashwood Stick
    assert(!can_craft_from_inventory(fishing, fishing_ground, 51, 63, 15));
    assert(fishing.add(41, 1)); // Rope
    assert(can_craft_from_inventory(fishing, fishing_ground, 51, 63, 15));
    assert(craft_from_inventory(fishing, fishing_ground, 51, 63, 15));
    assert(fishing.count(37) == 0 && fishing.count(41) == 0);
    assert(fishing_ground.count() == 1);
    assert(fishing_ground.entry(0)->item.item_id == 87);
    assert(fishing_ground.entry(0)->item.condition == 100);
    assert(fishing_ground.entry(0)->x == 51);
    assert(fishing_ground.entry(0)->y == 73);

    // Craft_tent: the source item description requires two Burlap Sacks,
    // followed by a 5-second wait, then a Canvas ground drop.
    InventorySystem canvas_inv;
    GroundLootSystem canvas_ground;
    assert(canvas_inv.add(40, 1));
    assert(!can_craft_from_inventory(canvas_inv, canvas_ground, 20, 30, 13));
    assert(canvas_inv.add(40, 1));
    assert(can_craft_from_inventory(canvas_inv, canvas_ground, 20, 30, 13));
    assert(craft_from_inventory(canvas_inv, canvas_ground, 20, 30, 13));
    assert(canvas_inv.count(40) == 0);
    assert(canvas_ground.count() == 1);
    assert(canvas_ground.entry(0)->item.item_id == 117);
    assert(canvas_ground.entry(0)->x == 20);
    assert(canvas_ground.entry(0)->y == 30);

    // Craft_xiaoyinqi: Water Bottle + Duct Tape, 5-second source wait.
    InventorySystem suppress_inv;
    GroundLootSystem suppress_ground;
    assert(suppress_inv.add(54, 1));
    assert(!can_craft_from_inventory(suppress_inv, suppress_ground, 3, 4, 14));
    assert(suppress_inv.add(39, 1));
    assert(can_craft_from_inventory(suppress_inv, suppress_ground, 3, 4, 14));
    assert(craft_from_inventory(suppress_inv, suppress_ground, 3, 4, 14));
    assert(suppress_inv.count(54) == 0 && suppress_inv.count(39) == 0);
    assert(suppress_inv.count(127) == 1);
    assert(suppress_ground.count() == 0);

    // Verify source-exact laser grip and Molotov ingredients and that crafted
    // items enter the inventory (not an invented ground-drop recipe).
    constexpr std::uint16_t source_inputs[3][2] = {
        {124, 111}, // Laser Sight + AK Grip -> Laser AK Grip
        {124, 112}, // Laser Sight + Rail Grip -> Laser Rail Grip
        {25, 20},   // Whiskey + Flare -> Molotov Cocktail
    };
    constexpr std::uint16_t expected_outputs[3] = {147,148,102};
    for (std::size_t n = 0; n < 3; ++n) {
        const auto* recipe = source_craft_recipe(10+n);
        assert(recipe && recipe->result_in_inventory);
        assert(recipe->ingredients[0].item_id == source_inputs[n][0]);
        assert(recipe->ingredients[1].item_id == source_inputs[n][1]);
        InventorySystem inv;
        GroundLootSystem loot;
        assert(inv.add(source_inputs[n][0], 1));
        assert(!can_craft_from_inventory(inv, loot, 20, 40, 10+n));
        assert(inv.add(source_inputs[n][1], 1));
        assert(can_craft_from_inventory(inv, loot, 20, 40, 10+n));
        assert(craft_from_inventory(inv, loot, 20, 40, 10+n));
        assert(inv.count(source_inputs[n][0]) == 0);
        assert(inv.count(source_inputs[n][1]) == 0);
        assert(inv.count(expected_outputs[n]) == 1);
        assert(loot.count() == 0);
    }

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

    // Source item_activate2: three rags must be in ONE selected stack.
    // The 3DS recipe panel chooses an eligible stack without merging stacks.
    InventorySystem rags_split;
    GroundLootSystem rags_ground;
    assert(rags_split.add_to_slot(0, 26, 2, 0, 71));
    assert(rags_split.add_to_slot(1, 26, 1, 0, 54));
    assert(!can_craft_from_inventory(rags_split, rags_ground, 0, 0, 9));
    assert(!craft_from_inventory(rags_split, rags_ground, 0, 0, 9));
    assert(rags_split.count(26) == 3 && rags_split.count(41) == 0);
    assert(rags_split.add_to_slot(2, 26, 3, 0, 63));
    assert(can_craft_from_inventory(rags_split, rags_ground, 0, 0, 9));
    assert(craft_from_inventory(rags_split, rags_ground, 0, 0, 9));
    assert(rags_split.count(41) == 1);
    assert(rags_split.count(26) == 3);
    assert(rags_split.slot(0)->count == 2 && rags_split.slot(0)->condition == 71);
    assert(rags_split.slot(1)->count == 1 && rags_split.slot(1)->condition == 54);
    assert(rags_split.slot(2)->item_id == 41);
    assert(rags_ground.count() == 0);

    // Source max stack for Rags is exactly 3. Consuming one full Rags
    // stack frees its slot, so conversion works even in a full inventory.
    InventorySystem rags_full;
    GroundLootSystem rags_full_ground;
    assert(rags_full.add_to_slot(0, 26, 3));
    for (std::size_t i = 1; i < rags_full.capacity(); ++i)
        assert(rags_full.add_to_slot(i, 42, 1));
    assert(can_craft_from_inventory(rags_full, rags_full_ground, 0, 0, 9));
    assert(craft_from_inventory(rags_full, rags_full_ground, 0, 0, 9));
    assert(rags_full.slot(0)->item_id == 41 && rags_full.slot(0)->count == 1);
    assert(rags_full.count(26) == 0 && rags_full_ground.count() == 0);

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
