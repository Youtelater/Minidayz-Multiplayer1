#include "minidayz/equipment.hpp"
#include "minidayz/generated/item_wear_catalog.hpp"

#include <cassert>
#include <cmath>

using namespace minidayz;
using namespace minidayz::items;

int main() {
    EquipmentSystem equipment;
    EquipmentSlot slot{};
    std::uint8_t repair_gain = 0;
    assert(source_item_repair_gain(39, SourceRepairTarget::pants, repair_gain) && repair_gain == 15);
    assert(source_item_repair_gain(39, SourceRepairTarget::outerwear, repair_gain) && repair_gain == 15);
    assert(source_item_repair_gain(39, SourceRepairTarget::backpack, repair_gain) && repair_gain == 15);
    assert(source_item_repair_gain(39, SourceRepairTarget::vest, repair_gain) && repair_gain == 15);
    assert(source_item_repair_gain(122, SourceRepairTarget::pants, repair_gain) && repair_gain == 25);
    assert(source_item_repair_gain(122, SourceRepairTarget::outerwear, repair_gain) && repair_gain == 25);
    assert(source_item_repair_gain(122, SourceRepairTarget::backpack, repair_gain) && repair_gain == 25);
    assert(source_item_repair_gain(122, SourceRepairTarget::vest, repair_gain) && repair_gain == 25);
    assert(source_item_repair_gain(35, SourceRepairTarget::helmet, repair_gain) && repair_gain == 50);
    assert(source_item_repair_gain(35, SourceRepairTarget::helmet_lite, repair_gain) && repair_gain == 25);
    assert(source_item_repair_gain(35, SourceRepairTarget::pants, repair_gain) && repair_gain == 50);
    assert(source_item_repair_gain(35, SourceRepairTarget::outerwear, repair_gain) && repair_gain == 50);
    assert(!source_item_repair_gain(35, SourceRepairTarget::backpack, repair_gain));
    assert(EquipmentSystem::source_slot_for_item(251, slot));
    assert(slot == EquipmentSlot::head);
    assert(EquipmentSystem::source_slot_for_item(303, slot));
    assert(slot == EquipmentSlot::pants);
    assert(EquipmentSystem::source_slot_for_item(355, slot));
    assert(slot == EquipmentSlot::outerwear);
    assert(EquipmentSystem::source_slot_for_item(402, slot));
    assert(slot == EquipmentSlot::vest);
    assert(EquipmentSystem::source_slot_for_item(158, slot));
    assert(slot == EquipmentSlot::hands);
    assert(!EquipmentSystem::source_slot_for_item(137, slot)); // gloves are not a source equipment slot
    assert(!EquipmentSystem::source_slot_for_item(140, slot)); // no source footwear slot
    assert(!EquipmentSystem::source_slot_for_item(456, slot)); // civilian tent

    InventorySystem inventory;
    assert(inventory.add(251, 1));
    assert(inventory.add(264, 1));
    assert(inventory.add(18, 1));
    assert(equipment.equip_from_inventory(inventory, 0, EquipmentSlot::head));
    assert(equipment.equipped(EquipmentSlot::head) == 251);
    assert(inventory.count(251) == 0);
    assert(!equipment.equip_from_inventory(inventory, 1, EquipmentSlot::pants));

    // Replacing headgear returns the old item atomically.
    assert(equipment.equip_from_inventory(inventory, 1, EquipmentSlot::head));
    assert(equipment.equipped(EquipmentSlot::head) == 264);
    assert(inventory.count(251) == 1);
    assert(equipment.unequip_to_inventory(inventory, EquipmentSlot::head, 1));
    assert(equipment.equipped(EquipmentSlot::head) == 0);
    assert(inventory.count(264) == 1);

    const auto saved = equipment.snapshot();
    EquipmentSystem restored;
    assert(restored.restore(saved));
    auto invalid = saved;
    invalid.item_ids[static_cast<std::size_t>(EquipmentSlot::pants)] = 251;
    assert(!restored.restore(invalid));
    assert(restored.equipped(EquipmentSlot::head) == 0);

    // Original map instance variables supply exact flat armor protection.
    SourceItemProtection protection{};
    assert(source_item_protection(251, protection));
    assert(protection.warmth == 2.0f);
    assert(protection.helmet == 4.0f);
    assert(source_item_protection(355, protection));
    assert(protection.warmth == 7.0f);
    assert(protection.outerwear == 2.0f);
    assert(source_item_protection(402, protection));
    assert(protection.vest == 3.0f);
    assert(source_item_protection(303, protection));
    assert(protection.warmth == 7.0f);
    assert(protection.pants == 2.0f);
    assert(source_item_protection(408, protection));
    assert(protection.vest == 20.0f);
    assert(!source_item_protection(450, protection));

    EquipmentSlot source_slot{};
    assert(EquipmentSystem::source_slot_for_item(410, source_slot));
    assert(source_slot == EquipmentSlot::vest);
    assert(EquipmentSystem::source_slot_for_item(411, source_slot));
    assert(source_slot == EquipmentSlot::vest);
    assert(EquipmentSystem::source_slot_for_item(412, source_slot));
    assert(source_slot == EquipmentSlot::vest);
    assert(source_item_protection(410, protection));
    assert(protection.vest == 10.0f);
    assert(source_item_protection(411, protection));
    assert(protection.vest == 20.0f);
    assert(EquipmentSystem::source_slot_for_item(409, source_slot));
    assert(source_slot == EquipmentSlot::outerwear);
    InventorySystem cbrn_inventory;
    EquipmentSystem cbrn_equipment;
    assert(cbrn_inventory.add(410, 1));
    assert(cbrn_equipment.equip_from_inventory(
        cbrn_inventory, 0, EquipmentSlot::vest));
    assert(cbrn_equipment.protection().vest == 10.0f);

    EquipmentSystem armor;
    InventorySystem armor_inventory;
    assert(armor_inventory.add(251, 1));
    assert(armor_inventory.add(355, 1));
    assert(armor_inventory.add(402, 1));
    assert(armor_inventory.add(303, 1));
    assert(armor.equip_from_inventory(armor_inventory, 0, EquipmentSlot::head));
    assert(armor.equip_from_inventory(armor_inventory, 1, EquipmentSlot::outerwear));
    assert(armor.equip_from_inventory(armor_inventory, 2, EquipmentSlot::vest));
    assert(armor.equip_from_inventory(armor_inventory, 3, EquipmentSlot::pants));
    const auto armor_values = armor.protection();
    assert(armor_values.helmet == 4.0f);
    assert(armor_values.outerwear == 2.0f);
    assert(armor_values.vest == 3.0f);
    assert(armor_values.pants == 2.0f);
    assert(std::fabs(armor.temperature_offset() - (5.0f / 7.0f)) < 0.0001f);
    PlayerState warm_state{};
    warm_state.temperature = 0.0f;
    PlayerSystem warm_player(warm_state);
    armor.apply_temperature(warm_player, 2.0f, -10.0f, 10.0f);
    assert(std::fabs(warm_player.state().temperature - (2.0f + 5.0f / 7.0f)) < 0.0001f);
    armor.apply_temperature(warm_player, 2.0f, -10.0f, 10.0f);
    assert(std::fabs(warm_player.state().temperature - (2.0f + 5.0f / 7.0f)) < 0.0001f);
    warm_player.adjust_temperature(20.0f, -10.0f, 10.0f);
    armor.apply_temperature(warm_player, 2.0f, -10.0f, 10.0f);
    assert(std::fabs(warm_player.state().temperature - (2.0f + 5.0f / 7.0f)) < 0.0001f);
    // The source event reassigns the absolute target every update; the
    // independent heat adjustment must not accumulate across frames.

    PlayerState equipment_damage_state{};
    equipment_damage_state.health = 100.0f;
    PlayerSystem equipment_damage_player(equipment_damage_state);
    armor.apply_damage(equipment_damage_player, 20.0f);
    assert(equipment_damage_player.state().health == 91.0f);

    PlayerState protected_state{};
    protected_state.health = 100.0f;
    protected_state.alive = true;
    PlayerSystem protected_player(protected_state);
    protected_player.apply_damage(20.0f, armor_values);
    assert(protected_player.state().health == 91.0f);
    protected_player.apply_damage(5.0f, armor_values);
    assert(protected_player.state().health == 91.0f);

    // Original item warmth and protection values remain flat when the
    // separate native condition field changes; no proportional scaling exists
    // in the original Construct event logic.
    assert(armor.damage_condition(EquipmentSlot::head, 50));
    const auto worn_armor = armor.protection();
    assert(worn_armor.helmet == 4.0f);
    assert(worn_armor.outerwear == 2.0f);
    assert(worn_armor.vest == 3.0f);
    assert(worn_armor.pants == 2.0f);
    assert(std::fabs(armor.temperature_offset() - (5.0f / 7.0f)) < 0.0001f);
    assert(armor.damage_condition(EquipmentSlot::pants, 50));
    assert(std::fabs(armor.temperature_offset() - (5.0f / 7.0f)) < 0.0001f);
    assert(armor.damage_condition(EquipmentSlot::vest, 100));
    assert(armor.equipped(EquipmentSlot::vest) == 0);
    assert(armor.protection().vest == 0.0f);

    // Item condition survives equipping, snapshots and unequipping.
    InventorySystem condition_inventory;
    EquipmentSystem condition_equipment;
    assert(condition_inventory.add(251, 1, 0, 65));
    assert(condition_equipment.equip_from_inventory(
        condition_inventory, 0, EquipmentSlot::head));
    assert(condition_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] == 65);
    EquipmentSystem condition_restored;
    const auto conditioned_equipment_snapshot = condition_equipment.snapshot();
    assert(condition_restored.restore(conditioned_equipment_snapshot));
    auto bad_equipment_condition = conditioned_equipment_snapshot;
    bad_equipment_condition.conditions[static_cast<std::size_t>(EquipmentSlot::head)] = 101;
    assert(!condition_restored.restore(bad_equipment_condition));
    assert(condition_restored.unequip_to_inventory(
        condition_inventory, EquipmentSlot::head, 0));
    assert(condition_inventory.slot(0)->condition == 65);

    assert(condition_equipment.damage_condition(EquipmentSlot::head, 0.25f));
    assert(std::fabs(condition_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] - 64.75f) < 0.0001f);
    EquipmentSystem fractional_equipment_restore;
    assert(fractional_equipment_restore.restore(condition_equipment.snapshot()));
    assert(std::fabs(fractional_equipment_restore.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] - 64.75f) < 0.0001f);
    assert(condition_equipment.damage_condition(EquipmentSlot::head, 25));
    assert(std::fabs(condition_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] - 39.75f) < 0.0001f);
    assert(condition_equipment.damage_condition(EquipmentSlot::head, 39.75f));
    assert(condition_equipment.equipped(EquipmentSlot::head) == 0);
    assert(!condition_equipment.damage_condition(EquipmentSlot::head, 1));

    // Clothing temperature is an absolute source target: Day_cold plus the
    // exact equipment expression, independent of frame delta.
    PlayerState temperature_state{};
    temperature_state.health = 100.0f;
    PlayerSystem temperature_player(temperature_state);
    EquipmentSystem temperature_equipment;
    temperature_equipment.apply_temperature(temperature_player, 7.0f, -10.0f, 10.0f);
    assert(temperature_player.state().temperature == 7.0f);

    // Repair uses an absolute source-derived value and never resurrects a broken item.
    EquipmentSystem repair_equipment;
    InventorySystem repair_source;
    assert(repair_source.add(251, 1, 0, 35));
    assert(repair_equipment.equip_from_inventory(
        repair_source, 0, EquipmentSlot::head));
    assert(repair_equipment.restore_condition(EquipmentSlot::head, 80));
    assert(repair_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] == 80);
    assert(!repair_equipment.restore_condition(EquipmentSlot::head, 80));
    assert(!repair_equipment.restore_condition(EquipmentSlot::head, 34));
    assert(!repair_equipment.restore_condition(EquipmentSlot::head, 101));
    assert(repair_equipment.damage_condition(EquipmentSlot::head, 80));
    assert(repair_equipment.equipped(EquipmentSlot::head) == 0);
    assert(!repair_equipment.restore_condition(EquipmentSlot::head, 100));

    // Original outerwear repair events: duct tape +15, epoxy glue +25,
    // sewing kit +50, with the source's 100-condition cap and item use.
    EquipmentSystem repaired_outerwear;
    InventorySystem repair_materials;
    assert(repair_materials.add(355, 1, 0, 40));
    assert(repair_materials.add(39, 1));   // Duct Tape
    assert(repair_materials.add(122, 1));  // Epoxy Glue
    assert(repair_materials.add(35, 1));   // Sewing Kit
    assert(repaired_outerwear.equip_from_inventory(
        repair_materials, 0, EquipmentSlot::outerwear));
    assert(repaired_outerwear.repair_from_inventory(repair_materials, 1, SourceRepairTarget::outerwear));
    assert(repaired_outerwear.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::outerwear)] == 55);
    assert(repair_materials.count(39) == 0);
    assert(repaired_outerwear.repair_from_inventory(repair_materials, 2, SourceRepairTarget::outerwear));
    assert(repaired_outerwear.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::outerwear)] == 80);
    assert(repair_materials.count(122) == 0);
    assert(repaired_outerwear.repair_from_inventory(repair_materials, 3, SourceRepairTarget::outerwear));
    assert(repaired_outerwear.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::outerwear)] == 100);
    assert(repair_materials.count(35) == 0);
    assert(!repaired_outerwear.repair_from_inventory(repair_materials, 3, SourceRepairTarget::outerwear));

    // Repair clamps at 100 and never consumes a material for full or broken gear.
    EquipmentSystem clamped_outerwear;
    InventorySystem clamp_materials;
    assert(clamp_materials.add(355, 1, 0, 95));
    assert(clamp_materials.add(39, 1));
    assert(clamped_outerwear.equip_from_inventory(
        clamp_materials, 0, EquipmentSlot::outerwear));
    assert(clamped_outerwear.repair_from_inventory(clamp_materials, 1, SourceRepairTarget::outerwear));
    assert(clamped_outerwear.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::outerwear)] == 100);
    assert(clamp_materials.count(39) == 0);

    EquipmentSystem broken_outerwear;
    InventorySystem broken_materials;
    assert(broken_materials.add(355, 1, 0, 0));
    assert(broken_materials.add(39, 1));
    assert(broken_outerwear.equip_from_inventory(
        broken_materials, 0, EquipmentSlot::outerwear));
    assert(!broken_outerwear.repair_from_inventory(broken_materials, 1, SourceRepairTarget::outerwear));
    assert(broken_materials.count(39) == 1);

    // All source repair menu variants route to the proper equipment slot.
    EquipmentSystem helmet_repair;
    InventorySystem helmet_materials;
    assert(helmet_materials.add(251, 1, 0, 40));
    assert(helmet_materials.add(35, 1));
    assert(helmet_repair.equip_from_inventory(helmet_materials, 0, EquipmentSlot::head));
    assert(helmet_repair.repair_from_inventory(
        helmet_materials, 1, SourceRepairTarget::helmet));
    assert(helmet_repair.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] == 90);

    EquipmentSystem light_helmet_repair;
    InventorySystem light_helmet_materials;
    assert(light_helmet_materials.add(251, 1, 0, 40));
    assert(light_helmet_materials.add(35, 1));
    assert(light_helmet_repair.equip_from_inventory(
        light_helmet_materials, 0, EquipmentSlot::head));
    assert(light_helmet_repair.repair_from_inventory(
        light_helmet_materials, 1, SourceRepairTarget::helmet_lite));
    assert(light_helmet_repair.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] == 65);

    EquipmentSystem pants_repair;
    InventorySystem pants_materials;
    assert(pants_materials.add(303, 1, 0, 40));
    assert(pants_materials.add(39, 1));
    assert(pants_repair.equip_from_inventory(pants_materials, 0, EquipmentSlot::pants));
    assert(pants_repair.repair_from_inventory(
        pants_materials, 1, SourceRepairTarget::pants));
    assert(pants_repair.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::pants)] == 55);

    EquipmentSystem vest_repair;
    InventorySystem vest_materials;
    assert(vest_materials.add(402, 1, 0, 40));
    assert(vest_materials.add(122, 1));
    assert(vest_repair.equip_from_inventory(vest_materials, 0, EquipmentSlot::vest));
    assert(vest_repair.repair_from_inventory(
        vest_materials, 1, SourceRepairTarget::vest));
    assert(vest_repair.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::vest)] == 65);

    // Backpack repairs share this public equipment repair entry point but
    // mutate the backpack condition owned by InventorySystem.
    EquipmentSystem backpack_repair_dispatch;
    InventorySystem backpack_repair_inventory;
    assert(backpack_repair_inventory.add(450, 1, 0, 40));
    assert(backpack_repair_inventory.add(39, 1));
    assert(backpack_repair_inventory.add(122, 1));
    assert(backpack_repair_inventory.equip_backpack_from_slot(0));
    assert(backpack_repair_dispatch.repair_from_inventory(
        backpack_repair_inventory, 1, SourceRepairTarget::backpack));
    assert(backpack_repair_inventory.equipped_backpack_condition() == 55);
    assert(backpack_repair_inventory.count(39) == 0);
    assert(backpack_repair_dispatch.repair_from_inventory(
        backpack_repair_inventory, 2, SourceRepairTarget::backpack));
    assert(backpack_repair_inventory.equipped_backpack_condition() == 80);
    assert(backpack_repair_inventory.count(122) == 0);
    assert(!backpack_repair_dispatch.repair_from_inventory(
        backpack_repair_inventory, 2, SourceRepairTarget::backpack));

    // Failed replacement must leave both inventory and equipment unchanged.
    EquipmentSystem atomic;
    InventorySystem full;
    assert(atomic.restore(EquipmentSnapshot{{251, 0, 0, 0, 0, 0, 0}}));
    for (std::uint16_t id = 1; id <= 15; ++id) assert(full.add(id, 1));
    assert(full.add(264, 1) == false);
    // Original Construct condition-write events, not estimated weapon wear.
    float wear = -1.0f;
    assert(source_condition_wear_loss(2098275646157213ULL, false, 0.0f, wear));
    assert(std::fabs(wear - 0.25f) < 0.0001f);
    assert(source_condition_wear_loss(2098275646157213ULL, true, 0.0f, wear));
    assert(wear == 0.0f);
    assert(source_condition_wear_loss(251602875520018ULL, false, 0.0f, wear));
    assert(wear == 1.0f);
    assert(source_condition_wear_loss(8036449108462062ULL, false, 0.0f, wear));
    assert(std::fabs(wear - 0.2f) < 0.0001f);
    assert(source_condition_wear_loss(682686209618980ULL, false, 0.5f, wear));
    assert(wear == 3.0f);
    assert(source_condition_wear_loss(682686209618980ULL, true, 0.5f, wear));
    assert(wear == 0.0f);
    assert(!source_condition_wear_loss(682686209618980ULL, false, 1.0f, wear));
    assert(wear == 0.0f);
    assert(!source_condition_wear_loss(0, false, 0.0f, wear));
    assert(wear == 0.0f);

    EquipmentSystem source_wear_equipment;
    InventorySystem source_wear_inventory;
    assert(source_wear_inventory.add_to_slot(0, 244, 1, 0, 75.0f));
    assert(source_wear_equipment.equip_from_inventory(
        source_wear_inventory, 0, EquipmentSlot::hands));
    assert(source_wear_equipment.apply_source_wear(875054325568454ULL, false));
    assert(std::fabs(source_wear_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::hands)] - 74.75f) < 0.0001f);
    assert(!source_wear_equipment.apply_source_wear(875054325568454ULL, true));
    assert(!source_wear_equipment.apply_source_wear(0, false));
    assert(source_wear_equipment.apply_source_wear(682686209618980ULL, false, 0.5f));
    assert(std::fabs(source_wear_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::hands)] - 71.75f) < 0.0001f);
    assert(source_wear_equipment.apply_source_wear(251602875520018ULL, false));
    assert(std::fabs(source_wear_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::hands)] - 70.75f) < 0.0001f);
    assert(source_wear_equipment.apply_source_wear(8036449108462062ULL, false));
    assert(std::fabs(source_wear_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::hands)] - 70.55f) < 0.0001f);
    assert(source_wear_equipment.apply_source_wear(251602875520018ULL, false));
    assert(source_wear_equipment.apply_source_wear(2098275646157213ULL, false));
    assert(source_wear_equipment.damage_condition(EquipmentSlot::hands, 100.0f));
    assert(!source_wear_equipment.apply_source_wear(2098275646157213ULL, false));

    // Regenerated original Construct source actions: action IDs are stable,
    // unlike enclosing event UIDs which may be duplicated by the source.
    assert(kSourceWearRuleCount >= 100);
    float source_action_loss = 77.0f;
    assert(source_condition_wear_action(
        744011060201717ULL, false, 0.0f, 0.0f, source_action_loss));
    assert(std::fabs(source_action_loss - 0.25f) < 0.0001f);
    assert(source_condition_wear_action(
        744011060201717ULL, true, 0.0f, 0.0f, source_action_loss));
    assert(source_action_loss == 0.0f);
    assert(source_condition_wear_action(
        7229337118015925ULL, false, 0.0f, 0.0f, source_action_loss));
    assert(std::fabs(source_action_loss - 0.4f) < 0.0001f);
    assert(source_condition_wear_action(
        228602990115248ULL, false, 0.0f, 0.0f, source_action_loss));
    assert(source_action_loss == 1.0f);
    assert(source_condition_wear_action(
        449456536338930ULL, false, 0.75f, 0.0f, source_action_loss));
    assert(source_action_loss == 3.0f);  // floor(random(2,4))
    assert(!source_condition_wear_action(
        449456536338930ULL, false, 1.0f, 0.0f, source_action_loss));
    assert(source_action_loss == 0.0f);
    assert(!source_condition_wear_action(
        123456789ULL, false, 0.0f, 0.0f, source_action_loss));
    assert(source_action_loss == 0.0f);

    // All original generated rules must be strictly sorted and unique.
    for (std::size_t i = 1; i < kSourceWearRuleCount; ++i) {
        assert(kSourceWearRules[i - 1].action_uid <
               kSourceWearRules[i].action_uid);
    }
    EquipmentSystem generated_wear_equipment;
    InventorySystem generated_wear_inventory;
    assert(generated_wear_inventory.add_to_slot(0, 244, 1, 0, 80.0f));
    assert(generated_wear_equipment.equip_from_inventory(
        generated_wear_inventory, 0, EquipmentSlot::hands));
    assert(generated_wear_equipment.apply_source_wear_action(
        744011060201717ULL, false));
    assert(std::fabs(generated_wear_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::hands)] - 79.75f) < 0.0001f);
    assert(!generated_wear_equipment.apply_source_wear_action(
        744011060201717ULL, true));

    return 0;
}
