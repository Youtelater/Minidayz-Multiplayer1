#include "minidayz/ground_loot.hpp"

#include <cmath>

namespace minidayz::items {
namespace {

bool valid_ground_item(const GroundItem& item) noexcept {
    return item.item_id != 0 && item.count != 0 &&
        source_item_exists(item.item_id) &&
        (item.condition == kUseSourceItemCondition || item.condition <= 100) &&
        item.count <= source_max_stack(item.item_id);
}

} // namespace

bool GroundLootSystem::add(const GroundItem& item, float x, float y) noexcept {
    if (count_ >= entries_.size() || !valid_ground_item(item) ||
        !std::isfinite(x) || !std::isfinite(y)) return false;
    entries_[count_++] = {item, x, y};
    return true;
}

bool GroundLootSystem::drop_from_inventory(
    InventorySystem& inventory, std::size_t slot, std::uint16_t amount,
    float x, float y) noexcept {
    if (count_ >= entries_.size() || !std::isfinite(x) || !std::isfinite(y))
        return false;
    GroundItem dropped{};
    if (!inventory.drop(slot, amount, dropped)) return false;
    entries_[count_++] = {dropped, x, y};
    return true;
}

bool GroundLootSystem::drop_equipped_backpack(
    InventorySystem& inventory, float x, float y) noexcept {
    if (count_ >= entries_.size() || !std::isfinite(x) || !std::isfinite(y))
        return false;
    GroundItem dropped{};
    if (!inventory.drop_equipped_backpack(dropped)) return false;
    entries_[count_++] = {dropped, x, y};
    return true;
}

const GroundLootEntry* GroundLootSystem::nearest(
    float x, float y, float radius) const noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(radius) || radius < 0.0f) return nullptr;
    const float radius_squared = radius * radius;
    float best_squared = radius_squared;
    const GroundLootEntry* best = nullptr;
    for (std::size_t i = 0; i < count_; ++i) {
        const float dx = entries_[i].x - x;
        const float dy = entries_[i].y - y;
        const float distance_squared = dx * dx + dy * dy;
        if (distance_squared <= best_squared) {
            best_squared = distance_squared;
            best = &entries_[i];
        }
    }
    return best;
}

bool GroundLootSystem::pickup_nearest(
    InventorySystem& inventory, float x, float y, float radius) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(radius) || radius < 0.0f) return false;
    const float radius_squared = radius * radius;
    std::size_t best_index = count_;
    float best_squared = radius_squared;
    for (std::size_t i = 0; i < count_; ++i) {
        const float dx = entries_[i].x - x;
        const float dy = entries_[i].y - y;
        const float distance_squared = dx * dx + dy * dy;
        if (distance_squared <= best_squared) {
            best_squared = distance_squared;
            best_index = i;
        }
    }
    if (best_index == count_) return false;
    GroundItem candidate = entries_[best_index].item;
    if (!inventory.pickup(candidate)) return false;
    for (std::size_t i = best_index + 1; i < count_; ++i)
        entries_[i - 1] = entries_[i];
    entries_[--count_] = {};
    return true;
}

const GroundLootEntry* GroundLootSystem::entry(std::size_t index) const noexcept {
    return index < count_ ? &entries_[index] : nullptr;
}

GroundLootSnapshot GroundLootSystem::snapshot() const noexcept {
    GroundLootSnapshot result{};
    result.count = static_cast<std::uint16_t>(count_);
    for (std::size_t i = 0; i < count_; ++i) result.entries[i] = entries_[i];
    return result;
}

bool GroundLootSystem::restore(const GroundLootSnapshot& saved) noexcept {
    if (saved.count > saved.entries.size()) return false;
    for (std::size_t i = 0; i < saved.count; ++i) {
        const auto& entry = saved.entries[i];
        if (!valid_ground_item(entry.item) || !std::isfinite(entry.x) ||
            !std::isfinite(entry.y)) return false;
    }
    for (std::size_t i = saved.count; i < saved.entries.size(); ++i)
        if (!saved.entries[i].item.empty()) return false;
    entries_ = saved.entries;
    count_ = saved.count;
    return true;
}

} // namespace minidayz::items
