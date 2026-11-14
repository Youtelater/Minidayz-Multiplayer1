#include "minidayz/item_use.hpp"
#include "minidayz/generated/item_use_effect_catalog.hpp"

#include <algorithm>

namespace minidayz::items {

bool source_item_use_effect(std::uint16_t item_id, bool secondary,
                            ItemUseEffect& effect) noexcept {
    std::size_t first = 0;
    std::size_t last = kItemUseEffectDefinitionCount;
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        const auto& row = kItemUseEffectDefinitions[middle];
        if (row.item_id < item_id ||
            (row.item_id == item_id && row.secondary < secondary))
            first = middle + 1;
        else
            last = middle;
    }
    if (first >= kItemUseEffectDefinitionCount) return false;
    const auto& row = kItemUseEffectDefinitions[first];
    if (row.item_id != item_id || row.secondary != secondary) return false;
    effect = {row.hunger, row.thirst, row.health, row.temperature};
    return true;
}

bool use_item_from_inventory(InventorySystem& inventory,
                             std::size_t inventory_slot,
                             PlayerSystem& player,
                             const PlayerRules& rules,
                             bool secondary) noexcept {
    const auto* item = inventory.slot(inventory_slot);
    if (!item || item->empty() || !player.state().alive) return false;

    ItemUseEffect effect{};
    if (!source_item_use_effect(item->item_id, secondary, effect)) return false;
    if (!inventory.remove(inventory_slot, 1)) return false;

    if (effect.hunger != 0.0f && rules.max_hunger > 0.0f)
        player.adjust_hunger(effect.hunger, rules.max_hunger);
    if (effect.thirst != 0.0f && rules.max_thirst > 0.0f)
        player.adjust_thirst(effect.thirst, rules.max_thirst);
    if (effect.health > 0.0f && rules.max_health > 0.0f)
        player.heal(effect.health, rules.max_health);
    else if (effect.health < 0.0f)
        player.apply_damage(-effect.health);
    if (effect.temperature != 0.0f &&
        rules.max_temperature > rules.min_temperature)
        player.adjust_temperature(effect.temperature, rules.min_temperature,
                                  rules.max_temperature);
    return true;
}

} // namespace minidayz::items
