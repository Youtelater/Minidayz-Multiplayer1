#include "minidayz/equipment.hpp"
#include "minidayz/generated/item_equipment_catalog.hpp"
#include "minidayz/item_repair.hpp"
#include "minidayz/source_wear.hpp"
#include "minidayz/generated/mp_quick_pistol_catalog.hpp"

#include <cmath>

#include <algorithm>

namespace minidayz::items {

bool EquipmentSystem::source_slot_for_item(std::uint16_t id,
                                           EquipmentSlot& slot) noexcept {
    // The original Player_get_hit and Drop_* events identify helmet,
    // outerwear, vest and pants as separate protection slots. The original
    // The source Inventory GUI has DIFFERENT cards for held melee tools
    // and ranged firearms. Preserve the original two simultaneous slots;
    // earlier native code incorrectly collapsed every gun into Hands.
    if ((id >= 133 && id <= 136) || (id >= 250 && id <= 277) ||
        id == 748 || id == 786 || id == 787) {
        slot = EquipmentSlot::head;
        return true;
    }
    if ((id >= 350 && id <= 368) || id == 702 || id == 703 ||
        id == 409 || (id >= 413 && id <= 415) || id == 711 || id == 713) {
        slot = EquipmentSlot::outerwear;
        return true;
    }
    if ((id >= 400 && id <= 405) || id == 408 ||
        (id >= 410 && id <= 412)) {
        slot = EquipmentSlot::vest;
        return true;
    }
    if (id >= 300 && id <= 307) {
        slot = EquipmentSlot::pants;
        return true;
    }
    // Exact source item categories from root l_eng_items.xml:
    // firearms 151..199 and 230..246 (including Romero 77 Hatchet);
    // the Flare Gun / launchers and alternate Romero occupy ranged.
    if ((id >= 151 && id <= 199) || (id >= 230 && id <= 246) ||
        id == 103 || id == 107 || id == 108 || id == 785) {
        slot = EquipmentSlot::weapon;
        return true;
    }
    // Original melee/tools 200..216 (Pipe Wrench ... Torch).
    if (id >= 200 && id <= 216) {
        slot = EquipmentSlot::hands;
        return true;
    }
    return false;
}

bool EquipmentSystem::item_fits_slot(std::uint16_t id,
                                     EquipmentSlot slot) noexcept {
    EquipmentSlot actual{};
    return source_slot_for_item(id, actual) && actual == slot;
}

std::uint16_t EquipmentSystem::equipped(EquipmentSlot slot) const noexcept {
    const auto index = static_cast<std::size_t>(slot);
    return index < state_.item_ids.size() ? state_.item_ids[index] : 0;
}

bool source_item_protection(std::uint16_t item_id,
                            SourceItemProtection& protection) noexcept {
    std::size_t first = 0;
    std::size_t last = kSourceItemProtectionCount;
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (kSourceItemProtection[middle].item_id < item_id)
            first = middle + 1;
        else
            last = middle;
    }
    if (first >= kSourceItemProtectionCount ||
        kSourceItemProtection[first].item_id != item_id)
        return false;
    protection = kSourceItemProtection[first];
    return true;
}

bool EquipmentSystem::restore(const EquipmentSnapshot& saved) noexcept {
    EquipmentSnapshot normalized = saved;
    // Migrate the first native save's provisional feet/weapon slots into the
    // original game's outerwear/hands slots when those are otherwise empty.
    const auto feet = static_cast<std::size_t>(EquipmentSlot::feet);
    const auto weapon = static_cast<std::size_t>(EquipmentSlot::weapon);
    const auto outerwear = static_cast<std::size_t>(EquipmentSlot::outerwear);
    const auto hands = static_cast<std::size_t>(EquipmentSlot::hands);
    if (normalized.item_ids[feet] != 0) {
        if (normalized.item_ids[outerwear] != 0) return false;
        normalized.item_ids[outerwear] = normalized.item_ids[feet];
        normalized.conditions[outerwear] = normalized.conditions[feet];
        normalized.item_ids[feet] = 0;
        normalized.conditions[feet] = 100;
    }
    // Older 3DS v19 saves incorrectly held firearms in Hands, because
    // their native GUI had no separate source weapon slot. Detect the
    // actual source item category and migrate without deleting durability.
    // Also migrate any provisional legacy melee item in reserved Weapon.
    // Older native saves may have both legacy categories reversed. If
    // both items have valid source categories, migrate the pair atomically
    // rather than rejecting the save or silently dropping either item.
    EquipmentSlot weapon_type{},hands_type{};
    const bool weapon_is_melee=normalized.item_ids[weapon]!=0 &&
        source_slot_for_item(normalized.item_ids[weapon],weapon_type) &&
        weapon_type==EquipmentSlot::hands;
    const bool hands_is_rifle=normalized.item_ids[hands]!=0 &&
        source_slot_for_item(normalized.item_ids[hands],hands_type) &&
        hands_type==EquipmentSlot::weapon;
    if(weapon_is_melee && hands_is_rifle){
        std::swap(normalized.item_ids[weapon],normalized.item_ids[hands]);
        std::swap(normalized.conditions[weapon],normalized.conditions[hands]);
    }else if(weapon_is_melee){
        if(normalized.item_ids[hands]!=0)return false;
        normalized.item_ids[hands]=normalized.item_ids[weapon];
        normalized.conditions[hands]=normalized.conditions[weapon];
        normalized.item_ids[weapon]=0;
        normalized.conditions[weapon]=100;
    }else if(hands_is_rifle){
        if(normalized.item_ids[weapon]!=0)return false;
        normalized.item_ids[weapon]=normalized.item_ids[hands];
        normalized.conditions[weapon]=normalized.conditions[hands];
        normalized.item_ids[hands]=0;
        normalized.conditions[hands]=100;
    }
    for (std::size_t i = 0; i < normalized.item_ids.size(); ++i) {
        if (normalized.conditions[i] > 100) return false;
        if (normalized.item_ids[i] == 0) continue;
        const auto slot = static_cast<EquipmentSlot>(i);
        if (!item_fits_slot(normalized.item_ids[i], slot)) return false;
        for (std::size_t j = i + 1; j < normalized.item_ids.size(); ++j)
            if (normalized.item_ids[i] == normalized.item_ids[j]) return false;
    }
    state_ = normalized;
    fractions_ = {};
    return true;
}

bool EquipmentSystem::restore_with_fraction(
    const EquipmentSnapshot& saved,
    const EquipmentFractionSnapshot& fractions) noexcept {
    EquipmentSystem candidate = *this;
    if (!candidate.restore(saved)) return false;
    auto normalized = fractions;
    // The original save's reserved feet/weapon indices were migrated to
    // outerwear/hands by restore(), so relocate their exact wear too.
    const auto feet = static_cast<std::size_t>(EquipmentSlot::feet);
    const auto weapon = static_cast<std::size_t>(EquipmentSlot::weapon);
    const auto hands = static_cast<std::size_t>(EquipmentSlot::hands);
    if (saved.item_ids[feet] != 0) {
        normalized.fractional_loss[static_cast<std::size_t>(EquipmentSlot::outerwear)] =
            normalized.fractional_loss[feet];
        normalized.fractional_loss[feet] = 0;
    }
    EquipmentSlot weapon_type{},hands_type{};
    const bool weapon_is_melee=saved.item_ids[weapon]!=0 &&
        source_slot_for_item(saved.item_ids[weapon],weapon_type) &&
        weapon_type==EquipmentSlot::hands;
    const bool hands_is_rifle=saved.item_ids[hands]!=0 &&
        source_slot_for_item(saved.item_ids[hands],hands_type) &&
        hands_type==EquipmentSlot::weapon;
    if(weapon_is_melee && hands_is_rifle){
        std::swap(normalized.fractional_loss[weapon],
                  normalized.fractional_loss[hands]);
    }else if(weapon_is_melee){
        normalized.fractional_loss[hands]=normalized.fractional_loss[weapon];
        normalized.fractional_loss[weapon]=0;
    }else if(hands_is_rifle){
        normalized.fractional_loss[weapon]=normalized.fractional_loss[hands];
        normalized.fractional_loss[hands]=0;
    }
    for(std::size_t i=0;i<kEquipmentSlotCount;++i) {
        const auto fraction=normalized.fractional_loss[i];
        if (fraction>=100 ||
            (fraction!=0 && (candidate.state_.item_ids[i]==0 ||
                             candidate.state_.conditions[i]==0)))
            return false;
    }
    candidate.fractions_ = normalized;
    *this = candidate;
    return true;
}

PlayerArmorProtection EquipmentSystem::protection() const noexcept {
    // The original Construct Player_get_hit path reads the flat protection
    // instance variables. Item condition is persisted separately; the source
    // does not multiply protection by condition.
    PlayerArmorProtection result{};
    SourceItemProtection item{};
    if (source_item_protection(equipped(EquipmentSlot::head), item))
        result.helmet = item.helmet;
    if (source_item_protection(equipped(EquipmentSlot::outerwear), item))
        result.outerwear = item.outerwear;
    if (source_item_protection(equipped(EquipmentSlot::vest), item))
        result.vest = item.vest;
    if (source_item_protection(equipped(EquipmentSlot::pants), item))
        result.pants = item.pants;
    return result;
}

void EquipmentSystem::apply_damage(PlayerSystem& player, float amount) const noexcept {
    player.apply_damage(amount, protection());
}

float EquipmentSystem::temperature_offset() const noexcept {
    // Original source event UID 4824763499434787 evaluates
    // Player_temperature = Day_cold - ((var29 + var28) + var30).
    // These are outerwear, helmet and pants warmth. No division is present.
    SourceItemProtection item{};
    float helmet_warmth = 0.0f;
    float outerwear_warmth = 0.0f;
    float pants_warmth = 0.0f;
    if (source_item_protection(equipped(EquipmentSlot::head), item))
        helmet_warmth = item.warmth;
    if (source_item_protection(equipped(EquipmentSlot::outerwear), item))
        outerwear_warmth = item.warmth;
    if (source_item_protection(equipped(EquipmentSlot::pants), item))
        pants_warmth = item.warmth;
    return outerwear_warmth + helmet_warmth + pants_warmth;
}

float EquipmentSystem::day_cooling(float day_cold) const noexcept {
    return std::isfinite(day_cold) ? day_cold - temperature_offset() : 0.0f;
}

ClothingPockets EquipmentSystem::clothing_pockets() const noexcept {
    const auto has_storage=[&](EquipmentSlot slot) {
        return equipped(slot)!=0 && state_.conditions[static_cast<std::size_t>(slot)]>0;
    };
    return {has_storage(EquipmentSlot::outerwear),has_storage(EquipmentSlot::pants),
            has_storage(EquipmentSlot::vest)};
}

bool EquipmentSystem::equip_from_inventory(
    InventorySystem& inventory, std::size_t inventory_slot,
    EquipmentSlot equipment_slot) noexcept {
    const auto slot_index = static_cast<std::size_t>(equipment_slot);
    const auto* incoming = inventory.slot(inventory_slot);
    if (slot_index >= state_.item_ids.size() || !incoming ||
        incoming->empty() || incoming->count != 1 ||
        !item_fits_slot(incoming->item_id, equipment_slot))
        return false;

    auto next_inventory=inventory;
    auto next_equipment=*this;
    next_equipment.state_.item_ids[slot_index]=incoming->item_id;
    next_equipment.state_.conditions[slot_index]=incoming->condition;
    next_equipment.fractions_.fractional_loss[slot_index]=incoming->fractional_loss;
    const auto outgoing_id=state_.item_ids[slot_index];
    if (!next_inventory.remove(inventory_slot,1) ||
        !next_inventory.change_clothing_pockets(next_equipment.clothing_pockets()) ||
        (outgoing_id!=0 && !next_inventory.add(outgoing_id,1,0,
            state_.conditions[slot_index],fractions_.fractional_loss[slot_index])))
        return false;
    inventory=next_inventory;
    *this=next_equipment;
    return true;
}

bool EquipmentSystem::damage_condition(
    EquipmentSlot slot, std::uint8_t loss) noexcept {
    return damage_condition_hundredths(slot,static_cast<std::uint16_t>(loss)*100u);
}

bool EquipmentSystem::damage_condition_hundredths(
    EquipmentSlot slot, std::uint16_t loss_hundredths) noexcept {
    const auto i=static_cast<std::size_t>(slot);
    if (i>=kEquipmentSlotCount || state_.item_ids[i]==0 ||
        loss_hundredths==0) return false;
    const auto fraction=fractions_.fractional_loss[i];
    if (fraction>=100 || state_.conditions[i]*100u<fraction) return false;
    const auto exact=static_cast<std::uint16_t>(
        state_.conditions[i]*100u - fraction);
    if (loss_hundredths>=exact) {
        state_.item_ids[i]=0;
        state_.conditions[i]=100;
        fractions_.fractional_loss[i]=0;
    } else {
        const auto remaining=static_cast<std::uint16_t>(exact-loss_hundredths);
        state_.conditions[i]=static_cast<std::uint8_t>((remaining+99u)/100u);
        fractions_.fractional_loss[i]=static_cast<std::uint8_t>(
            state_.conditions[i]*100u-remaining);
    }
    return true;
}

bool EquipmentSystem::apply_verified_wear_action(
    EquipmentSlot slot, std::uint64_t group_uid, std::uint64_t event_uid,
    std::uint64_t action_uid, bool source_guards_satisfied,
    bool perk_wxnj_active) noexcept {
    if(!source_guards_satisfied) return false;
    const auto* source=source_wear_action(group_uid,event_uid,action_uid);
    if(!source || (source->cancel_when_wxnj_active && perk_wxnj_active))
        return false;
    return damage_condition_hundredths(slot,source->loss_hundredths);
}

bool EquipmentSystem::restore_condition(
    EquipmentSlot slot, std::uint8_t condition) noexcept {
    const auto index = static_cast<std::size_t>(slot);
    if (index >= state_.item_ids.size() || state_.item_ids[index] == 0 ||
        condition > 100 ||
        static_cast<unsigned>(condition) * 100u <=
            static_cast<unsigned>(state_.conditions[index]) * 100u -
                fractions_.fractional_loss[index])
        return false;
    state_.conditions[index] = condition;
    fractions_.fractional_loss[index] = 0;
    return true;
}

bool EquipmentSystem::repair_from_inventory(
    InventorySystem& inventory, std::size_t inventory_slot,
    SourceRepairTarget target) noexcept {
    // Backpack condition is owned by InventorySystem, not an EquipmentSlot.
    // Route the original backpack repair actions to that authoritative state.
    if (target == SourceRepairTarget::backpack)
        return inventory.repair_equipped_backpack_from_inventory_slot(
            inventory_slot);

    const auto* material = inventory.slot(inventory_slot);
    if (!material || material->empty()) return false;

    EquipmentSlot equipment_slot{};
    switch (target) {
        case SourceRepairTarget::helmet:
        case SourceRepairTarget::helmet_lite:
            equipment_slot = EquipmentSlot::head;
            break;
        case SourceRepairTarget::outerwear:
            equipment_slot = EquipmentSlot::outerwear;
            break;
        case SourceRepairTarget::vest:
            equipment_slot = EquipmentSlot::vest;
            break;
        case SourceRepairTarget::pants:
            equipment_slot = EquipmentSlot::pants;
            break;
        case SourceRepairTarget::backpack:
        default:
            return false;
    }
    const auto equipment_index = static_cast<std::size_t>(equipment_slot);
    if (state_.item_ids[equipment_index] == 0) return false;
    const auto current = state_.conditions[equipment_index];
    // Original repair menu conditions require condition > 0 and < 100.
    const auto exact = static_cast<unsigned>(current) * 100u -
        fractions_.fractional_loss[equipment_index];
    if (exact == 0 || exact >= 10000u) return false;

    std::uint8_t gain = 0;
    if (!source_item_repair_gain(material->item_id, target, gain))
        return false;
    const auto repaired = static_cast<std::uint8_t>(
        std::min<unsigned>(100, static_cast<unsigned>(current) + gain));

    const auto inventory_before = inventory.snapshot();
    const auto equipment_before = state_;
    if (!inventory.remove(inventory_slot, 1)) return false;
    if (!restore_condition(equipment_slot, repaired)) {
        (void)inventory.restore(inventory_before);
        state_ = equipment_before;
        return false;
    }
    return true;
}

bool EquipmentSystem::unequip_to_inventory(
    InventorySystem& inventory, EquipmentSlot equipment_slot,
    std::size_t inventory_slot) noexcept {
    const auto slot_index = static_cast<std::size_t>(equipment_slot);
    if (slot_index >= state_.item_ids.size() ||
        state_.item_ids[slot_index] == 0)
        return false;
    const std::uint16_t outgoing_id = state_.item_ids[slot_index];
    const std::uint8_t outgoing_condition = state_.conditions[slot_index];
    const std::uint8_t outgoing_fraction = fractions_.fractional_loss[slot_index];
    auto next_inventory=inventory;
    auto next_equipment=*this;
    next_equipment.state_.item_ids[slot_index]=0;
    next_equipment.state_.conditions[slot_index]=100;
    next_equipment.fractions_.fractional_loss[slot_index]=0;
    if (!next_inventory.add_to_slot(inventory_slot,outgoing_id,1,0,
                                    outgoing_condition,outgoing_fraction) ||
        !next_inventory.change_clothing_pockets(next_equipment.clothing_pockets()) ||
        !next_inventory.slot_enabled(inventory_slot)) return false;
    inventory=next_inventory;
    *this=next_equipment;
    return true;
}


bool EquipmentSystem::restore_quick_pistol(
    const QuickPistolSnapshot& saved) noexcept {
    if(saved.condition>100 || saved.fractional_loss>=100)return false;
    if(saved.item_id==0){
        if(saved.condition!=100 || saved.fractional_loss!=0)return false;
    }else{
        if(!is_mp_quick_pistol(saved.item_id) ||
           !source_item_exists(saved.item_id) ||
           (saved.condition==0 && saved.fractional_loss!=0))
            return false;
    }
    quick_pistol_=saved;
    return true;
}

bool EquipmentSystem::equip_quick_pistol_from_inventory(
    InventorySystem& inventory,std::size_t index) noexcept {
    const auto* incoming=inventory.slot(index);
    if(!incoming || incoming->empty() || incoming->count!=1 ||
       !is_mp_quick_pistol(incoming->item_id))
        return false;
    const auto before=inventory.snapshot();
    const QuickPistolSnapshot old=quick_pistol_;
    const QuickPistolSnapshot next{
        incoming->item_id,incoming->condition,incoming->fractional_loss};
    if(!inventory.remove(index,1))return false;
    if(old.item_id && !inventory.add(old.item_id,1,0,
                                    old.condition,old.fractional_loss)){
        (void)inventory.restore(before);
        return false;
    }
    quick_pistol_=next;
    return true;
}

bool EquipmentSystem::unequip_quick_pistol_to_inventory(
    InventorySystem& inventory,std::size_t index) noexcept {
    if(!quick_pistol_.item_id)return false;
    if(!inventory.add_to_slot(index,quick_pistol_.item_id,1,0,
                               quick_pistol_.condition,
                               quick_pistol_.fractional_loss))
        return false;
    quick_pistol_={};
    return true;
}

bool EquipmentSystem::damage_quick_pistol_hundredths(
    std::uint16_t loss) noexcept {
    if(quick_pistol_.item_id==0 || loss==0)return false;
    const std::uint16_t exact=static_cast<std::uint16_t>(
        quick_pistol_.condition*100u-quick_pistol_.fractional_loss);
    if(loss>=exact){
        quick_pistol_={};
    }else{
        const auto remaining=static_cast<std::uint16_t>(exact-loss);
        quick_pistol_.condition=static_cast<std::uint8_t>((remaining+99u)/100u);
        quick_pistol_.fractional_loss=static_cast<std::uint8_t>(
            quick_pistol_.condition*100u-remaining);
    }
    return true;
}

} // namespace minidayz::items
