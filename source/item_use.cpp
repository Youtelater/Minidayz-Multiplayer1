#include "minidayz/item_use.hpp"
#include "minidayz/generated/item_use_effect_catalog.hpp"
#include "minidayz/generated/item_medical_catalog.hpp"
#include "minidayz/generated/item_secondary_crafting_catalog.hpp"

#include <algorithm>

namespace minidayz::items {

bool source_item_use_effect(std::uint16_t item_id, bool secondary,
                            ItemUseEffect& effect) noexcept {
    return source_item_use_effect(item_id, secondary, ItemUsePerks{}, effect);
}

bool source_item_use_effect(std::uint16_t item_id, bool secondary,
                            const ItemUsePerks& perks,
                            ItemUseEffect& effect) noexcept {
    // A failed lookup must never leak the previous item's effect through an
    // output parameter. Callers can safely reuse one effect object per frame.
    effect = {};
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
    if (perks.hamster_active) effect.hunger += row.hamster_hunger_bonus;
    if (perks.camel_active) effect.thirst += row.camel_thirst_bonus;
    return true;
}

bool use_item_from_inventory(InventorySystem& inventory,
                             std::size_t inventory_slot,
                             PlayerSystem& player,
                             const PlayerRules& rules,
                             bool secondary, const ItemUsePerks& perks,
                             ItemUseRuntime* delayed) noexcept {
    const auto* item = inventory.slot(inventory_slot);
    if (!item || item->empty() || !player.state().alive) return false;

    if (delayed && delayed->busy()) return false;
    // Original Newspapers and Scout Handbook item-activate events consume
    // one item, wait 5/10 seconds respectively and award 50/200 XP.
    // Without a scheduler, don't consume an item and discard its source XP.
    if (!secondary && (item->item_id == ItemUseRuntime::kOriginalNewspaperItemId ||
                       item->item_id == ItemUseRuntime::kOriginalScoutHandbookItemId)) {
        if (!delayed) return false;
        const auto item_id = item->item_id;
        const auto before = inventory.snapshot();
        if (!inventory.remove(inventory_slot, 1)) return false;
        if (!delayed->begin_reading(item_id)) {
            (void)inventory.restore(before);
            return false;
        }
        return true;
    }
    // Original item_activate2 explicitly reduces the selected Rags stack
    // by three then calls Check_space_inventory(-1, Rope). The 3DS native
    // inventory transaction is atomic; source capacity/condition rules apply.
    if (secondary && item->item_id == kSourceRagsToRopeInputId) {
        if (item->count < kSourceRagsToRopeRequiredCount) return false;
        const auto before = inventory.snapshot();
        if (!inventory.remove(inventory_slot, kSourceRagsToRopeRequiredCount))
            return false;
        if (!inventory.add(kSourceRagsToRopeOutputId, 1)) {
            (void)inventory.restore(before);
            return false;
        }
        return true;
    }

    ItemUseEffect effect{};
    if (!source_item_use_effect(item->item_id, secondary, perks, effect)) {
        // These cases are not stat deltas. The original item_activate nodes
        // directly reset the player's bleeding (var 22) or infection (var 35).
        // A bandage or rags can only be used when the source bleeding flag is
        // active; tetracycline's source branch has no such prerequisite.
        if (secondary) return false;
        for (const auto& rule : kSourceMedicalRules) {
            if (rule.item_id != item->item_id) continue;
            if (rule.action == SourceMedicalAction::stop_bleeding &&
                player.state().bleeding <= 0.0f) return false;
            if (delayed) {
                // Source consumes the material before its timed Wait action.
                if (!delayed->can_begin(rule.item_id)) return false;
                const auto previous = inventory.snapshot();
                if (!inventory.remove(inventory_slot, 1)) return false;
                if (!delayed->begin(rule.item_id)) {
                    (void)inventory.restore(previous);
                    return false;
                }
            } else {
                // Compatibility helper for callers without a native event
                // scheduler; the 3DS game always supplies one.
                if (!inventory.remove(inventory_slot, 1)) return false;
                if (rule.action == SourceMedicalAction::stop_bleeding)
                    player.set_bleeding(0.0f, rules.max_bleeding);
                else if (rule.action == SourceMedicalAction::stop_infection)
                    player.set_infection(0.0f, rules.max_infection);
            }
            return true;
        }
        return false;
    }
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
