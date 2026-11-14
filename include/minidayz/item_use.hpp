#pragma once

#include "minidayz/inventory.hpp"
#include "minidayz/player.hpp"

#include <cstdint>

namespace minidayz::items {

struct ItemUseEffect {
    float hunger{};
    float thirst{};
    float health{};
    float temperature{};
};

bool source_item_use_effect(std::uint16_t item_id, bool secondary,
                            ItemUseEffect& effect) noexcept;

// Executes the original Use (A) or Use2 (X) stat deltas for one inventory
// item. Items without a translated source activation effect are not consumed.
bool use_item_from_inventory(InventorySystem& inventory,
                             std::size_t inventory_slot,
                             PlayerSystem& player,
                             const PlayerRules& rules,
                             bool secondary = false) noexcept;

} // namespace minidayz::items
