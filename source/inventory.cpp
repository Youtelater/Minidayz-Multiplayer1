#include "minidayz/inventory.hpp"
#include "minidayz/generated/item_catalog.hpp"

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

bool InventorySystem::set_capacity(std::size_t capacity) noexcept {
    if (capacity < kInventorySlotCount || capacity > kMaximumInventorySlotCount)
        return false;
    for (std::size_t i = capacity; i < capacity_; ++i)
        if (!slots_[i].empty()) return false;
    capacity_ = capacity;
    return true;
}

bool InventorySystem::add(std::uint16_t item_id, std::uint16_t amount,
                          std::uint16_t max_stack) noexcept {
    if (!valid_item(item_id) || amount == 0 || max_stack == 0) return false;

    std::uint32_t available = 0;
    for (std::size_t i = 0; i < capacity_; ++i) {
        const auto& entry = slots_[i];
        if (entry.empty()) {
            available += max_stack;
        } else if (entry.item_id == item_id && entry.count < max_stack) {
            available += static_cast<std::uint32_t>(max_stack - entry.count);
        }
    }
    if (available < amount) return false;

    std::uint32_t remaining = amount;
    for (std::size_t i = 0; i < capacity_; ++i) {
        auto& entry = slots_[i];
        if (entry.item_id != item_id || entry.count >= max_stack) continue;
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
        entry.item_id = item_id;
        entry.count = static_cast<std::uint16_t>(added);
        remaining -= added;
        if (remaining == 0) return true;
    }
    return false;
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

bool InventorySystem::move(std::size_t from, std::size_t to,
                           std::uint16_t amount) noexcept {
    if (from >= capacity_ || to >= capacity_ || from == to ||
        slots_[from].empty()) return false;
    auto& source = slots_[from];
    auto& destination = slots_[to];
    const std::uint16_t moving = amount == 0 ? source.count : amount;
    if (moving == 0 || moving > source.count) return false;

    if (destination.empty()) {
        destination = {source.item_id, moving};
    } else if (destination.item_id == source.item_id) {
        if (moving > std::numeric_limits<std::uint16_t>::max() - destination.count)
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

} // namespace minidayz::items
