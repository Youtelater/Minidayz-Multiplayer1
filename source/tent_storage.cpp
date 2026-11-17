#include "minidayz/tent_storage.hpp"

#include <cmath>
#include <utility>

namespace minidayz::items {
namespace {
bool valid_stack(const InventorySlot& item) noexcept {
    return item.item_id != 0 && item.count != 0 &&
           source_item_exists(item.item_id) && item.condition <= 100 &&
           item.fractional_loss < 100 &&
           (item.fractional_loss == 0 || item.condition != 0) &&
           item.count <= source_max_stack(item.item_id);
}
}

bool TentStorageSystem::deploy_from_inventory(InventorySystem& inventory,
                                              std::size_t inventory_slot,
                                              float x, float y) noexcept {
    if (count_ >= tents_.size() || !std::isfinite(x) || !std::isfinite(y))
        return false;
    const auto* item = inventory.slot(inventory_slot);
    if (!item || item->item_id != kOriginalCivilianTentItemId ||
        item->empty() || item->condition > 100) return false;
    const auto condition = item->condition;
    if (!inventory.remove(inventory_slot, 1)) return false;
    tents_[count_] = {};
    tents_[count_].x = x;
    tents_[count_].y = y;
    tents_[count_].condition = condition;
    ++count_;
    return true;
}

bool TentStorageSystem::deploy_from_inventory_checked(
    InventorySystem& inventory, std::size_t inventory_slot,
    float x, float y, TentPlacementBlocked blocked,
    void* context, float clearance_radius) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(clearance_radius) ||
        clearance_radius < 0.0f || clearance_radius > 10000.0f)
        return false;
    // Validate before consuming the tent item. The four probe points are
    // native collision safety checks, not inferred Construct placement rules.
    const float r = clearance_radius;
    if (blocked && (blocked(context, x, y) ||
                    blocked(context, x - r, y - r) ||
                    blocked(context, x + r, y - r) ||
                    blocked(context, x - r, y + r) ||
                    blocked(context, x + r, y + r)))
        return false;
    const float separation = 2.0f * r;
    for (std::size_t i = 0; i < count_; ++i) {
        const float dx = tents_[i].x - x;
        const float dy = tents_[i].y - y;
        if (dx * dx + dy * dy < separation * separation)
            return false;
    }
    return deploy_from_inventory(inventory, inventory_slot, x, y);
}

std::size_t TentStorageSystem::nearest(float x, float y, float radius) const noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(radius) || radius < 0.0f) return count_;
    std::size_t closest = count_;
    float best = radius * radius;
    for (std::size_t i = 0; i < count_; ++i) {
        const float dx = tents_[i].x - x;
        const float dy = tents_[i].y - y;
        const float distance = dx * dx + dy * dy;
        if (distance <= best) { best = distance; closest = i; }
    }
    return closest;
}

bool TentStorageSystem::store_from_inventory(InventorySystem& inventory,
                                              std::size_t inventory_slot,
                                              std::size_t tent_index) noexcept {
    if (tent_index >= count_) return false;
    const auto* item = inventory.slot(inventory_slot);
    if (!item || !valid_stack(*item)) return false;
    const InventorySlot moving = *item;
    // Preflight the complete stack before consuming anything.
    auto updated = tents_[tent_index].items;
    std::uint32_t remaining = moving.count;
    const std::uint16_t limit = source_max_stack(moving.item_id);
    for (auto& stored : updated) {
        if (stored.item_id != moving.item_id ||
            stored.condition != moving.condition ||
            stored.fractional_loss != moving.fractional_loss ||
            stored.count >= limit)
            continue;
        const auto room = static_cast<std::uint32_t>(limit - stored.count);
        const auto amount = remaining < room ? remaining : room;
        stored.count = static_cast<std::uint16_t>(stored.count + amount);
        remaining -= amount;
        if (!remaining) break;
    }
    for (auto& stored : updated) {
        if (!remaining) break;
        if (!stored.empty()) continue;
        const auto amount = remaining < limit ? remaining : limit;
        stored = {moving.item_id, static_cast<std::uint16_t>(amount), moving.condition, moving.fractional_loss};
        remaining -= amount;
    }
    if (remaining || !inventory.remove(inventory_slot, moving.count)) return false;
    tents_[tent_index].items = updated;
    return true;
}

bool TentStorageSystem::take_to_inventory(InventorySystem& inventory,
                                           std::size_t tent_index,
                                           std::size_t tent_slot) noexcept {
    if (tent_index >= count_ || tent_slot >= kNativeTentStorageSlots)
        return false;
    auto& item = tents_[tent_index].items[tent_slot];
    if (!valid_stack(item) ||
        !inventory.add(item.item_id, item.count, 0, item.condition,
                       item.fractional_loss)) return false;
    item = {};
    return true;
}

const DeployedTent* TentStorageSystem::tent(std::size_t index) const noexcept {
    return index < count_ ? &tents_[index] : nullptr;
}

TentStorageSnapshot TentStorageSystem::snapshot() const noexcept {
    TentStorageSnapshot saved{};
    saved.count = static_cast<std::uint16_t>(count_);
    for (std::size_t i = 0; i < count_; ++i) saved.tents[i] = tents_[i];
    return saved;
}

bool TentStorageSystem::restore(const TentStorageSnapshot& saved) noexcept {
    if (saved.count > kNativeMaximumDeployedTents) return false;
    for (std::size_t i = 0; i < saved.tents.size(); ++i) {
        const auto& tent = saved.tents[i];
        if (i >= saved.count) {
            for (const auto& item : tent.items) if (!item.empty()) return false;
            continue;
        }
        if (!std::isfinite(tent.x) || !std::isfinite(tent.y) ||
            tent.condition > 100) return false;
        for (const auto& item : tent.items) {
            if ((item.item_id == 0) != (item.count == 0)) return false;
            if (item.empty()) {
                if (item.condition != 100) return false;
            } else if (!valid_stack(item)) return false;
        }
    }
    tents_ = saved.tents;
    count_ = saved.count;
    return true;
}
} // namespace minidayz::items
