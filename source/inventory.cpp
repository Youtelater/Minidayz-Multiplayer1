#include "minidayz/inventory.hpp"
#include "minidayz/generated/item_catalog.hpp"
#include "minidayz/generated/item_source_catalog.hpp"
#include "minidayz/generated/item_sprite_catalog.hpp"

#include <limits>

namespace minidayz::items {
namespace {

bool valid_item(std::uint16_t item_id) noexcept {
    if (item_id == 0) return false;
    std::size_t first = 0;
    std::size_t last = kItemDefinitionCount;
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        const auto candidate = kItemDefinitions[middle].id;
        if (candidate < item_id) first = middle + 1;
        else last = middle;
    }
    return first < kItemDefinitionCount &&
           kItemDefinitions[first].id == item_id &&
           kItemDefinitions[first].name[0] != '\0';
}

} // namespace

std::uint16_t source_max_stack(std::uint16_t item_id, bool perk_cybb) noexcept {
    return perk_cybb ? std::numeric_limits<std::uint16_t>::max()
                     : source_max_stack(item_id);
}

std::uint16_t source_construct_object(std::uint16_t item_id) noexcept {
    std::size_t first = 0;
    std::size_t last = kSourceItemDefinitionCount;
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (kSourceItemDefinitions[middle].id < item_id) first = middle + 1;
        else last = middle;
    }
    if (first == kSourceItemDefinitionCount ||
        kSourceItemDefinitions[first].id != item_id)
        return 65535;
    return kSourceItemDefinitions[first].construct_object;
}

std::uint16_t source_sprite_object(std::uint16_t item_id) noexcept {
    std::size_t first = 0;
    std::size_t last = kItemSpriteDefinitionCount;
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (kItemSpriteDefinitions[middle].item_id < item_id) first = middle + 1;
        else last = middle;
    }
    if (first == kItemSpriteDefinitionCount ||
        kItemSpriteDefinitions[first].item_id != item_id)
        return 65535;
    return kItemSpriteDefinitions[first].object;
}

bool is_backpack_item(std::uint16_t item_id) noexcept {
    // Exact backpack equipment IDs present in the original MDZItemTable.
    // Civilian Tent (456) and Supplies Stash (461) are not wearable packs.
    switch (item_id) {
        case 450: case 451: case 452: case 453: case 454: case 455:
        case 457: case 458: case 459: case 460: case 462:
        case 752: case 753: case 754:
            return true;
        default:
            return false;
    }
}

std::uint16_t source_max_stack(std::uint16_t item_id) noexcept {
    // The item-ID groups and baseline limits are taken from the original
    // Check_stack_size Construct function in data.deobfuscated.js.
    switch (item_id) {
        case 11: case 14: case 15: case 16: case 101:
            return 50;
        case 18: case 5: case 6: case 7: case 26: case 32: case 33:
        case 29: case 45: case 46: case 710: case 718: case 143: case 777:
            return 3;
        case 20: case 27: case 38: case 73: case 74: case 114: case 118:
            return 5;
        case 138: case 13:
            return 40;
        case 12: case 69: case 100:
            return 60;
        case 24: case 120: case 105: case 106: case 51: case 52: case 728:
            return 10;
        case 17: case 758:
            return 30;
        case 58: case 104:
            return 80;
        default:
            return 1;
    }
}

bool InventorySystem::set_capacity(std::size_t capacity) noexcept {
    if (capacity < kInventorySlotCount || capacity > kMaximumInventorySlotCount)
        return false;
    if (capacity == kInventorySlotCount && equipped_backpack_ != 0)
        return false;
    for (std::size_t i = capacity; i < capacity_; ++i)
        if (!slots_[i].empty()) return false;
    capacity_ = capacity;
    return true;
}

bool InventorySystem::equip_backpack_from_slot(
    std::size_t index, std::size_t source_capacity) noexcept {
    if (index >= capacity_ || !is_backpack_item(slots_[index].item_id) ||
        slots_[index].count != 1 || slots_[index].condition > 100 ||
        source_capacity <= kInventorySlotCount ||
        source_capacity > kMaximumInventorySlotCount)
        return false;

    const std::uint16_t incoming = slots_[index].item_id;
    const std::uint8_t incoming_condition = slots_[index].condition;
    const std::uint16_t outgoing = equipped_backpack_;
    const std::uint8_t outgoing_condition = equipped_backpack_condition_;

    std::size_t return_slot = index;
    if (outgoing != 0 && index >= source_capacity) {
        return_slot = source_capacity;
        for (std::size_t i = 0; i < source_capacity; ++i) {
            if (i != index && slots_[i].empty()) {
                return_slot = i;
                break;
            }
        }
        if (return_slot == source_capacity) return false;
    }

    for (std::size_t i = 0; i < capacity_; ++i) {
        if (i == index || (outgoing != 0 && i == return_slot)) continue;
        if (i >= source_capacity && !slots_[i].empty()) return false;
    }

    slots_[index] = {};
    if (outgoing != 0) slots_[return_slot] = {outgoing, 1, outgoing_condition};
    capacity_ = source_capacity;
    equipped_backpack_ = incoming;
    equipped_backpack_condition_ = incoming_condition;
    return true;
}

bool InventorySystem::unequip_backpack_to_slot(std::size_t index) noexcept {
    if (equipped_backpack_ == 0 || index >= kInventorySlotCount ||
        index >= capacity_ || !slots_[index].empty())
        return false;
    for (std::size_t i = kInventorySlotCount; i < capacity_; ++i)
        if (!slots_[i].empty()) return false;

    const std::uint16_t outgoing = equipped_backpack_;
    const std::uint8_t condition = equipped_backpack_condition_;
    capacity_ = kInventorySlotCount;
    equipped_backpack_ = 0;
    equipped_backpack_condition_ = 100;
    slots_[index] = {outgoing, 1, condition};
    return true;
}

bool InventorySystem::add(std::uint16_t item_id, std::uint16_t amount,
                          std::uint16_t max_stack,
                          std::uint8_t condition) noexcept {
    if (!valid_item(item_id) || amount == 0 || condition > 100) return false;
    if (max_stack == 0) max_stack = source_max_stack(item_id);
    if (max_stack == 0) return false;

    std::uint32_t available = 0;
    for (std::size_t i = 0; i < capacity_; ++i) {
        const auto& entry = slots_[i];
        if (entry.empty()) {
            available += max_stack;
        } else if (entry.item_id == item_id && entry.condition == condition &&
                   entry.count < max_stack) {
            available += static_cast<std::uint32_t>(max_stack - entry.count);
        }
    }
    if (available < amount) return false;

    std::uint32_t remaining = amount;
    for (std::size_t i = 0; i < capacity_; ++i) {
        auto& entry = slots_[i];
        if (entry.item_id != item_id || entry.condition != condition ||
            entry.count >= max_stack) continue;
        const auto room = static_cast<std::uint32_t>(max_stack - entry.count);
        const auto added = remaining < room ? remaining : room;
        entry.count = static_cast<std::uint16_t>(entry.count + added);
        remaining -= added;
        if (remaining == 0) return true;
    }
    for (std::size_t i = 0; i < capacity_; ++i) {
        auto& entry = slots_[i];
        if (!entry.empty()) continue;
        const auto added = remaining < max_stack ? remaining : max_stack;
        entry = {item_id, static_cast<std::uint16_t>(added), condition};
        remaining -= added;
        if (remaining == 0) return true;
    }
    return false;
}

bool InventorySystem::add_to_slot(std::size_t index, std::uint16_t item_id,
                                  std::uint16_t amount,
                                  std::uint16_t max_stack,
                                  std::uint8_t condition) noexcept {
    if (index >= capacity_ || !valid_item(item_id) || amount == 0 ||
        condition > 100)
        return false;
    if (max_stack == 0) max_stack = source_max_stack(item_id);
    if (max_stack == 0 || amount > max_stack) return false;
    auto& entry = slots_[index];
    if (entry.empty()) {
        entry = {item_id, amount, condition};
        return true;
    }
    if (entry.item_id != item_id || entry.condition != condition ||
        entry.count > max_stack || amount > max_stack - entry.count)
        return false;
    entry.count = static_cast<std::uint16_t>(entry.count + amount);
    return true;
}

bool InventorySystem::remove(std::size_t index,
                             std::uint16_t amount) noexcept {
    if (index >= capacity_ || amount == 0 ||
        slots_[index].empty() || amount > slots_[index].count) return false;
    auto& entry = slots_[index];
    entry.count = static_cast<std::uint16_t>(entry.count - amount);
    if (entry.count == 0) entry = {};
    return true;
}

bool InventorySystem::pickup(GroundItem& ground) noexcept {
    if (ground.empty() || (ground.item_id == 0) != (ground.count == 0) ||
        ground.condition > 100 || !valid_item(ground.item_id))
        return false;
    if (!add(ground.item_id, ground.count, 0, ground.condition)) return false;
    ground = {};
    return true;
}

bool InventorySystem::drop(std::size_t index, std::uint16_t amount,
                           GroundItem& ground) noexcept {
    if (index >= capacity_ || slots_[index].empty()) return false;
    if ((ground.item_id == 0) != (ground.count == 0)) return false;

    const auto& source = slots_[index];
    const std::uint16_t moving = amount == 0 ? source.count : amount;
    const std::uint16_t stack_limit = source_max_stack(source.item_id);
    if (moving == 0 || moving > source.count || moving > stack_limit)
        return false;

    if (!ground.empty()) {
        if (ground.item_id != source.item_id ||
            ground.condition != source.condition ||
            ground.count > stack_limit ||
            moving > stack_limit - ground.count)
            return false;
    }

    if (ground.empty()) {
        ground = {source.item_id, moving, source.condition};
    } else {
        ground.count = static_cast<std::uint16_t>(ground.count + moving);
    }
    auto& mutable_source = slots_[index];
    mutable_source.count = static_cast<std::uint16_t>(mutable_source.count - moving);
    if (mutable_source.count == 0) mutable_source = {};
    return true;
}

bool InventorySystem::move(std::size_t from, std::size_t to,
                           std::uint16_t amount,
                           std::uint16_t max_stack) noexcept {
    if (from >= capacity_ || to >= capacity_ || from == to ||
        slots_[from].empty()) return false;
    auto& source = slots_[from];
    auto& destination = slots_[to];
    const std::uint16_t moving = amount == 0 ? source.count : amount;
    if (moving == 0 || moving > source.count) return false;

    if (destination.empty()) {
        if (max_stack == 0) max_stack = source_max_stack(source.item_id);
        if (max_stack == 0 || moving > max_stack) return false;
        destination = {source.item_id, moving, source.condition};
    } else if (destination.item_id == source.item_id &&
               destination.condition == source.condition) {
        if (max_stack == 0) max_stack = source_max_stack(source.item_id);
        if (max_stack == 0 || destination.count > max_stack ||
            moving > max_stack - destination.count)
            return false;
        destination.count = static_cast<std::uint16_t>(destination.count + moving);
    } else {
        if (moving != source.count) return false;
        const InventorySlot temporary = destination;
        destination = source;
        source = temporary;
        return true;
    }

    source.count = static_cast<std::uint16_t>(source.count - moving);
    if (source.count == 0) source = {};
    return true;
}

const InventorySlot* InventorySystem::slot(std::size_t index) const noexcept {
    return index < capacity_ ? &slots_[index] : nullptr;
}

std::uint32_t InventorySystem::count(std::uint16_t item_id) const noexcept {
    std::uint32_t total = 0;
    for (std::size_t i = 0; i < capacity_; ++i) {
        const auto& entry = slots_[i];
        if (!entry.empty() && entry.item_id == item_id) total += entry.count;
    }
    return total;
}

std::size_t InventorySystem::occupied_slots() const noexcept {
    std::size_t total = 0;
    for (std::size_t i = 0; i < capacity_; ++i)
        if (!slots_[i].empty()) ++total;
    return total;
}

InventorySnapshot InventorySystem::snapshot() const noexcept {
    InventorySnapshot result{};
    result.slots = slots_;
    result.capacity = static_cast<std::uint16_t>(capacity_);
    result.equipped_backpack = equipped_backpack_;
    result.equipped_backpack_condition = equipped_backpack_condition_;
    return result;
}

bool InventorySystem::restore(const InventorySnapshot& saved) noexcept {
    if (saved.capacity < kInventorySlotCount ||
        saved.capacity > kMaximumInventorySlotCount ||
        saved.equipped_backpack_condition > 100)
        return false;
    if (saved.equipped_backpack != 0 &&
        (!is_backpack_item(saved.equipped_backpack) ||
         saved.capacity <= kInventorySlotCount))
        return false;
    if (saved.equipped_backpack == 0 &&
        saved.equipped_backpack_condition != 100)
        return false;

    for (std::size_t i = 0; i < kMaximumInventorySlotCount; ++i) {
        const auto& entry = saved.slots[i];
        if (i >= saved.capacity) {
            if (!entry.empty()) return false;
            continue;
        }
        if ((entry.item_id == 0) != (entry.count == 0)) return false;
        if (!entry.empty() &&
            (!valid_item(entry.item_id) || entry.condition > 100))
            return false;
    }

    slots_ = saved.slots;
    capacity_ = saved.capacity;
    equipped_backpack_ = saved.equipped_backpack;
    equipped_backpack_condition_ = saved.equipped_backpack_condition;
    return true;
}

} // namespace minidayz::items
