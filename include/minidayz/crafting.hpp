#pragma once

#include "minidayz/inventory.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/generated/item_crafting_catalog.hpp"

#include <cstddef>

namespace minidayz::items {

// Craft_1/4/5 emit Spawn_drop at player XY; Craft_3 returns an inventory item.
const SourceCraftRecipe* source_craft_recipe(std::size_t index) noexcept;

// Fail atomically if ingredients are absent, the ground list is full, or the
// output position is invalid. Output location follows its original event.
bool craft_from_inventory(InventorySystem& inventory,
                          GroundLootSystem& ground,
                          float player_x, float player_y,
                          std::size_t recipe_index) noexcept;
bool can_craft_from_inventory(const InventorySystem& inventory,
                              const GroundLootSystem& ground,
                              float player_x, float player_y,
                              std::size_t recipe_index) noexcept;

} // namespace minidayz::items
