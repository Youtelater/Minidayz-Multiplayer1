#include "minidayz/crafting.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

const SourceCraftRecipe* source_craft_recipe(std::size_t index) noexcept {
    return index < kSourceCraftRecipeCount ? &kSourceCraftRecipes[index] : nullptr;
}

bool craft_from_inventory(InventorySystem& inventory,
                          GroundLootSystem& ground,
                          float player_x, float player_y,
                          std::size_t recipe_index) noexcept {
    const auto* recipe = source_craft_recipe(recipe_index);
    if (!recipe || !source_item_exists(recipe->result_id)) return false;

    if (recipe->selected_stack_only) {
        // Original item_activate2 checks the SELECTED Rags stack, rather
        // than summing separate stacks. This crafting panel chooses the
        // first eligible selected-stack equivalent; the source-validated
        // secondary item action remains available from inventory Use2.
        const auto ingredient = recipe->ingredients[0];
        if (ingredient.count == 0 || recipe->ingredients[1].count != 0 ||
            !recipe->result_in_inventory ||
            recipe->preserve_second_ingredient) return false;
        for (std::size_t i = 0; i < inventory.capacity(); ++i) {
            const auto* slot = inventory.slot(i);
            if (!slot || slot->item_id != ingredient.item_id ||
                slot->count < ingredient.count) continue;
            const auto before = inventory.snapshot();
            if (!inventory.remove(i, ingredient.count)) return false;
            if (inventory.add(recipe->result_id, recipe->result_count))
                return true;
            (void)inventory.restore(before);
            return false;
        }
        return false;
    }

    for (const auto& ingredient : recipe->ingredients) {
        if (ingredient.count != 0 &&
            inventory.count(ingredient.item_id) < ingredient.count)
            return false;
    }

    const auto before = inventory.snapshot();
    const auto ground_before = ground.snapshot();
    // Craft_1/4/5 call Spawn_drop at player XY. Craft_3 instead calls
    // Check_space_inventory(-1,23), so its kit belongs in inventory.
    // The transaction rolls both stores back on any failure.
    if (!recipe->result_in_inventory &&
        !ground.add({recipe->result_id, recipe->result_count,
                     kUseSourceItemCondition}, player_x, player_y))
        return false;

    for (std::size_t part = 0; part < recipe->ingredients.size(); ++part) {
        if (part == 1 && recipe->preserve_second_ingredient) continue;
        const auto& ingredient = recipe->ingredients[part];
        std::uint16_t remaining = ingredient.count;
        for (std::size_t i = 0; i < inventory.capacity() && remaining != 0; ++i) {
            const auto* slot = inventory.slot(i);
            if (!slot || slot->empty() || slot->item_id != ingredient.item_id)
                continue;
            const std::uint16_t consumed = std::min(remaining, slot->count);
            if (!inventory.remove(i, consumed)) {
                (void)inventory.restore(before);
                (void)ground.restore(ground_before);
                return false;
            }
            remaining = static_cast<std::uint16_t>(remaining - consumed);
        }
        if (remaining != 0) {
            (void)inventory.restore(before);
            (void)ground.restore(ground_before);
            return false;
        }
    }
    if (recipe->result_in_inventory &&
        !inventory.add(recipe->result_id, recipe->result_count)) {
        (void)inventory.restore(before);
        (void)ground.restore(ground_before);
        return false;
    }
    return true;
}

bool can_craft_from_inventory(const InventorySystem& inventory,
                              const GroundLootSystem& ground,
                              float player_x, float player_y,
                              std::size_t recipe_index) noexcept {
    InventorySystem simulated = inventory;
    GroundLootSystem simulated_ground = ground;
    return craft_from_inventory(simulated, simulated_ground,
                                player_x, player_y, recipe_index);
}

} // namespace minidayz::items
