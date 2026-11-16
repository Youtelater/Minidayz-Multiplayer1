#pragma once

#include "minidayz/inventory.hpp"
#include "minidayz/generated/item_crafting_catalog.hpp"

#include <cstddef>

namespace minidayz::items {

// Only the recipes that can be translated without inventing item quantities
// or mechanics from the original root-level Construct/English tutorial.
const SourceCraftRecipe* source_craft_recipe(std::size_t index) noexcept;

// Crafting is atomic: consume all original ingredients and add the output or
// leave inventory identical, including stack counts and individual conditions.
bool craft_from_inventory(InventorySystem& inventory,
                          std::size_t recipe_index) noexcept;
bool can_craft_from_inventory(const InventorySystem& inventory,
                              std::size_t recipe_index) noexcept;

} // namespace minidayz::items
