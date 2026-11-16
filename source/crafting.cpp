#include "minidayz/crafting.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

const SourceCraftRecipe* source_craft_recipe(std::size_t index) noexcept {
    return index < kSourceCraftRecipeCount ? &kSourceCraftRecipes[index] : nullptr;
}

bool craft_from_inventory(InventorySystem& inventory,
                          std::size_t recipe_index) noexcept {
    const auto* recipe = source_craft_recipe(recipe_index);
    if (!recipe || !source_item_exists(recipe->result_id)) return false;

    // Validate before mutating any slot: no partial craft if an ingredient
    // is absent. A later failed result insertion restores the exact snapshot.
    for (const auto& ingredient : recipe->ingredients) {
        if (ingredient.count == 0 ||
            inventory.count(ingredient.item_id) < ingredient.count)
            return false;
    }

    const auto before = inventory.snapshot();
    for (const auto& ingredient : recipe->ingredients) {
        std::uint16_t remaining = ingredient.count;
        for (std::size_t i = 0; i < inventory.capacity() && remaining != 0; ++i) {
            const auto* slot = inventory.slot(i);
            if (!slot || slot->empty() || slot->item_id != ingredient.item_id)
                continue;
            const std::uint16_t consumed = std::min(remaining, slot->count);
            if (!inventory.remove(i, consumed)) {
                (void)inventory.restore(before);
                return false;
            }
            remaining = static_cast<std::uint16_t>(remaining - consumed);
        }
        if (remaining != 0) {
            (void)inventory.restore(before);
            return false;
        }
    }

    if (!inventory.add(recipe->result_id, recipe->result_count)) {
        (void)inventory.restore(before);
        return false;
    }
    return true;
}

bool can_craft_from_inventory(const InventorySystem& inventory,
                              std::size_t recipe_index) noexcept {
    InventorySystem simulated = inventory;
    return craft_from_inventory(simulated, recipe_index);
}

} // namespace minidayz::items
