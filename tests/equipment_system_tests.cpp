#include "minidayz/equipment.hpp"
#include "minidayz/weather_source.hpp"

#include <cassert>
#include <cmath>

using namespace minidayz;
using namespace minidayz::items;

int main() {
    // Exact original source Game_events difficulty weather values.
    const auto& novice_cold = source_weather_cold(SurvivalDifficulty::novice);
    const auto& regular_cold = source_weather_cold(SurvivalDifficulty::regular);
    const auto& veteran_cold = source_weather_cold(SurvivalDifficulty::veteran);
    assert(novice_cold.day == 6.0f && novice_cold.night == 1.0f &&
           novice_cold.rain == 1.0f);
    assert(regular_cold.day == 7.0f && regular_cold.night == 2.0f &&
           regular_cold.rain == 2.0f);
    assert(veteran_cold.day == 8.0f && veteran_cold.night == 3.0f &&
           veteran_cold.rain == 3.0f);
    assert(kSourceWeatherCold[3].day == 9.0f); // Legend source event

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
    assert(slot == EquipmentSlot::weapon); // original firearm card
    assert(EquipmentSystem::source_slot_for_item(206, slot));
    assert(slot == EquipmentSlot::hands); // original melee/pickaxe card
    assert(EquipmentSystem::source_slot_for_item(244, slot));
    assert(slot == EquipmentSlot::weapon); // Romero 77 Hatchet is a firearm
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
    assert(std::fabs(armor.temperature_offset() - 16.0f) < 0.0001f);
    assert(armor.day_cooling(7.0f) == -9.0f);
    assert(armor.day_cooling(6.0f) == -10.0f);

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
    assert(std::fabs(armor.temperature_offset() - 16.0f) < 0.0001f);
    assert(armor.damage_condition(EquipmentSlot::pants, 50));
    assert(std::fabs(armor.temperature_offset() - 16.0f) < 0.0001f);
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

    assert(condition_equipment.damage_condition(EquipmentSlot::head, 25));
    assert(condition_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::head)] == 40);
    assert(condition_equipment.damage_condition(EquipmentSlot::head, 40));
    assert(condition_equipment.equipped(EquipmentSlot::head) == 0);
    assert(!condition_equipment.damage_condition(EquipmentSlot::head, 1));

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
    // Source-authenticated subpercent damage must follow a held weapon
    // through equipping, saving and unequipping without rounding away 0.25%.
    InventorySystem fraction_inventory;
    EquipmentSystem fractional_gear;
    assert(fraction_inventory.add(244, 1, 0, 100));
    assert(fraction_inventory.damage_condition_hundredths(0, 25));
    assert(fractional_gear.equip_from_inventory(
        fraction_inventory,0,EquipmentSlot::weapon));
    const auto held=static_cast<std::size_t>(EquipmentSlot::weapon);
    assert(fractional_gear.snapshot().item_ids[held] == 244);
    assert(fractional_gear.snapshot().conditions[held] == 100);
    assert(fractional_gear.fraction_snapshot().fractional_loss[held] == 25);
    assert(!fractional_gear.apply_verified_wear_action(
        EquipmentSlot::weapon,6904772320561696ULL,875054325568454ULL,
        744011060201717ULL,false,false));
    assert(fractional_gear.apply_verified_wear_action(
        EquipmentSlot::weapon,6904772320561696ULL,875054325568454ULL,
        744011060201717ULL,true,false));
    assert(fractional_gear.fraction_snapshot().fractional_loss[held] == 50);
    assert(!fractional_gear.apply_verified_wear_action(
        EquipmentSlot::weapon,6904772320561696ULL,875054325568454ULL,
        744011060201717ULL,true,true));
    EquipmentSystem fraction_loaded;
    assert(fraction_loaded.restore_with_fraction(
        fractional_gear.snapshot(),fractional_gear.fraction_snapshot()));
    auto bad_fraction=fractional_gear.fraction_snapshot();
    bad_fraction.fractional_loss[held]=255;
    assert(!fraction_loaded.restore_with_fraction(
        fractional_gear.snapshot(),bad_fraction));
    assert(fraction_loaded.fraction_snapshot().fractional_loss[held] == 50);
    assert(fraction_loaded.unequip_to_inventory(
        fraction_inventory,EquipmentSlot::weapon,0));
    assert(fraction_inventory.slot(0)->item_id == 244);
    assert(fraction_inventory.slot(0)->condition == 100);
    assert(fraction_inventory.slot(0)->fractional_loss == 50);

    // The original inventory screenshot has a separate melee tool and
    // rifle card. Preserve BOTH across native equipment and save cycles.
    InventorySystem dual_inventory;
    EquipmentSystem dual_gear;
    assert(dual_inventory.add(206,1));
    assert(dual_inventory.add(158,1));
    assert(dual_gear.equip_from_inventory(dual_inventory,0,EquipmentSlot::hands));
    assert(dual_gear.equip_from_inventory(dual_inventory,1,EquipmentSlot::weapon));
    assert(dual_gear.equipped(EquipmentSlot::hands)==206);
    assert(dual_gear.equipped(EquipmentSlot::weapon)==158);
    EquipmentSystem dual_saved;
    assert(dual_saved.restore_with_fraction(
        dual_gear.snapshot(),dual_gear.fraction_snapshot()));
    assert(dual_saved.equipped(EquipmentSlot::hands)==206);
    assert(dual_saved.equipped(EquipmentSlot::weapon)==158);
    assert(dual_saved.unequip_to_inventory(dual_inventory,EquipmentSlot::weapon,0));
    assert(dual_inventory.count(158)==1);
    assert(dual_saved.equipped(EquipmentSlot::hands)==206);
    // Existing save v19 stored an active firearm in Hands. It must now
    // migrate to the correct source weapon slot with its fractional wear.
    EquipmentSnapshot legacy_gun{};
    legacy_gun.item_ids[static_cast<std::size_t>(EquipmentSlot::hands)]=158;
    legacy_gun.conditions[static_cast<std::size_t>(EquipmentSlot::hands)]=68;
    EquipmentFractionSnapshot legacy_wear{};
    legacy_wear.fractional_loss[static_cast<std::size_t>(EquipmentSlot::hands)]=42;
    EquipmentSystem migrated;
    assert(migrated.restore_with_fraction(legacy_gun,legacy_wear));
    assert(migrated.equipped(EquipmentSlot::hands)==0);
    assert(migrated.equipped(EquipmentSlot::weapon)==158);
    assert(migrated.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::weapon)]==68);
    assert(migrated.fraction_snapshot().fractional_loss[
        static_cast<std::size_t>(EquipmentSlot::weapon)]==42);

    // Some legacy native records had both firearm and melee categories
    // reversed. Restore both atomically with their independent condition
    // and sub-percent wear instead of rejecting a valid survivor save.
    EquipmentSnapshot reversed{};
    reversed.item_ids[static_cast<std::size_t>(EquipmentSlot::hands)]=158;
    reversed.conditions[static_cast<std::size_t>(EquipmentSlot::hands)]=71;
    reversed.item_ids[static_cast<std::size_t>(EquipmentSlot::weapon)]=206;
    reversed.conditions[static_cast<std::size_t>(EquipmentSlot::weapon)]=84;
    EquipmentFractionSnapshot reversed_wear{};
    reversed_wear.fractional_loss[static_cast<std::size_t>(EquipmentSlot::hands)]=23;
    reversed_wear.fractional_loss[static_cast<std::size_t>(EquipmentSlot::weapon)]=67;
    EquipmentSystem swapped;
    assert(swapped.restore_with_fraction(reversed,reversed_wear));
    assert(swapped.equipped(EquipmentSlot::hands)==206);
    assert(swapped.equipped(EquipmentSlot::weapon)==158);
    assert(swapped.snapshot().conditions[static_cast<std::size_t>(
        EquipmentSlot::hands)]==84);
    assert(swapped.snapshot().conditions[static_cast<std::size_t>(
        EquipmentSlot::weapon)]==71);
    assert(swapped.fraction_snapshot().fractional_loss[
        static_cast<std::size_t>(EquipmentSlot::hands)]==67);
    assert(swapped.fraction_snapshot().fractional_loss[
        static_cast<std::size_t>(EquipmentSlot::weapon)]==23);

    // A 99.75%-exact jacket displays as 100% with the source byte
    // compatibility field, but it is still legitimately repairable.
    EquipmentSystem subpercent_jacket;
    InventorySystem jacket_inventory;
    assert(jacket_inventory.add(350,1,0,100,25));
    assert(jacket_inventory.add(39,1));
    assert(subpercent_jacket.equip_from_inventory(
        jacket_inventory,0,EquipmentSlot::outerwear));
    assert(subpercent_jacket.repair_from_inventory(
        jacket_inventory,1,SourceRepairTarget::outerwear));
    const auto outer=static_cast<std::size_t>(EquipmentSlot::outerwear);
    assert(subpercent_jacket.snapshot().conditions[outer]==100);
    assert(subpercent_jacket.fraction_snapshot().fractional_loss[outer]==0);
    assert(jacket_inventory.count(39)==0);

    // MP quick pistol uses a THIRD independent weapon slot, whose allowed
    // item IDs are extracted from root mp_kit.js and mp_players.js.
    InventorySystem quick_bag;
    EquipmentSystem quick_gear;
    assert(quick_bag.add(151,1,0,88,37));  // source MP handgun
    assert(quick_bag.add(170,1,0,92,23));  // source MP handgun
    assert(quick_bag.add(158,1));           // rifle, not MP handgun
    assert(!quick_gear.equip_quick_pistol_from_inventory(quick_bag,2));
    assert(quick_bag.count(158)==1);
    assert(quick_gear.equip_quick_pistol_from_inventory(quick_bag,0));
    assert(quick_gear.quick_pistol()==151);
    assert(quick_gear.quick_pistol_snapshot().condition==88);
    assert(quick_gear.quick_pistol_snapshot().fractional_loss==37);
    assert(quick_gear.equip_quick_pistol_from_inventory(quick_bag,1));
    assert(quick_gear.quick_pistol()==170);
    assert(quick_bag.count(151)==1);
    assert(quick_gear.damage_quick_pistol_hundredths(625));
    assert(quick_gear.quick_pistol_snapshot().condition==86);
    assert(quick_gear.quick_pistol_snapshot().fractional_loss==48);
    EquipmentSystem restored_quick;
    assert(restored_quick.restore_quick_pistol(
        quick_gear.quick_pistol_snapshot()));
    assert(restored_quick.quick_pistol()==170);
    assert(!restored_quick.restore_quick_pistol({158,100,0}));
    assert(restored_quick.quick_pistol()==170);
    assert(!restored_quick.restore_quick_pistol({170,101,0}));
    assert(restored_quick.unequip_quick_pistol_to_inventory(quick_bag,3));
    assert(restored_quick.quick_pistol()==0);
    assert(quick_bag.count(170)==1);
    assert(quick_bag.slot(3)->condition==86);
    assert(quick_bag.slot(3)->fractional_loss==48);
    restored_quick.clear();
    assert(restored_quick.quick_pistol_snapshot().condition==100);

    return 0;
}
