#include "minidayz/stash_storage.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace minidayz::items {
namespace {
bool valid(const InventorySlot& item) noexcept {
    return item.item_id != 0 && item.count != 0 &&
        source_item_exists(item.item_id) && item.condition <= 100 &&
        item.fractional_loss < 100 &&
        (item.condition > 0 || item.fractional_loss == 0) &&
        item.count <= source_max_stack(item.item_id);
}
}

bool StashStorageSystem::deploy_from_inventory_checked(
    InventorySystem& inventory, std::size_t inventory_slot,
    float x, float y, TentPlacementBlocked blocked, void* context,
    float radius) noexcept {
    if (count_ >= stashes_.size() || !std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(radius) || radius < 0.0f || radius > 10000.0f)
        return false;
    const auto* held = inventory.slot(inventory_slot);
    if (!held || held->item_id != kOriginalSuppliesStashItemId ||
        held->empty() || held->condition > 100 || held->fractional_loss >= 100)
        return false;
    if (blocked && (blocked(context,x,y) || blocked(context,x-radius,y-radius) ||
                    blocked(context,x+radius,y-radius) ||
                    blocked(context,x-radius,y+radius) ||
                    blocked(context,x+radius,y+radius)))
        return false;
    const auto safe_separation = radius * 2.0f;
    for (std::size_t i=0;i<count_;++i) {
        const auto dx=stashes_[i].x-x;
        const auto dy=stashes_[i].y-y;
        if (dx*dx+dy*dy < safe_separation*safe_separation)
            return false;
    }
    const auto condition=held->condition;
    if (!inventory.remove(inventory_slot,1)) return false;
    stashes_[count_] = {};
    stashes_[count_].x=x;
    stashes_[count_].y=y;
    stashes_[count_].condition=condition;
    ++count_;
    return true;
}

bool StashStorageSystem::store_from_inventory(
    InventorySystem& inventory,std::size_t slot,std::size_t stash_index) noexcept {
    if (stash_index>=count_) return false;
    const auto* item=inventory.slot(slot);
    if (!item || !valid(*item)) return false;
    const auto moving=*item;
    const auto limit=source_max_stack(moving.item_id);
    auto proposed=stashes_[stash_index].items;
    std::uint32_t remaining=moving.count;
    for (auto& destination:proposed) {
        if (destination.empty() || destination.item_id!=moving.item_id ||
            destination.condition!=moving.condition ||
            destination.fractional_loss!=moving.fractional_loss ||
            destination.count>=limit) continue;
        const auto amount=std::min<std::uint32_t>(
            remaining,limit-destination.count);
        destination.count=static_cast<std::uint16_t>(destination.count+amount);
        remaining-=amount;
        if (!remaining) break;
    }
    for (auto& destination:proposed) {
        if (!remaining) break;
        if (!destination.empty()) continue;
        const auto amount=std::min<std::uint32_t>(remaining,limit);
        destination = {moving.item_id,static_cast<std::uint16_t>(amount),
                       moving.condition,moving.fractional_loss};
        remaining-=amount;
    }
    if (remaining || !inventory.remove(slot,moving.count)) return false;
    stashes_[stash_index].items=proposed;
    return true;
}

bool StashStorageSystem::take_to_inventory(
    InventorySystem& inventory,std::size_t stash_index,
    std::size_t stash_slot) noexcept {
    if (stash_index>=count_ || stash_slot>=kSourceSuppliesStashSlots)
        return false;
    auto& item=stashes_[stash_index].items[stash_slot];
    if (!valid(item) || !inventory.add(
            item.item_id,item.count,0,item.condition,item.fractional_loss))
        return false;
    item={};
    return true;
}

std::size_t StashStorageSystem::nearest(
    float x,float y,float radius) const noexcept {
    if (!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(radius)||
        radius<0.0f) return count_;
    std::size_t found=count_;
    float best=radius*radius;
    for(std::size_t i=0;i<count_;++i) {
        const auto dx=stashes_[i].x-x;
        const auto dy=stashes_[i].y-y;
        const auto distance=dx*dx+dy*dy;
        if (distance<=best) {best=distance;found=i;}
    }
    return found;
}

const DeployedStash* StashStorageSystem::stash(std::size_t index) const noexcept {
    return index<count_?&stashes_[index]:nullptr;
}

StashStorageSnapshot StashStorageSystem::snapshot() const noexcept {
    StashStorageSnapshot output{};
    output.count=static_cast<std::uint16_t>(count_);
    for(std::size_t i=0;i<count_;++i)output.stashes[i]=stashes_[i];
    return output;
}

bool StashStorageSystem::restore(const StashStorageSnapshot& saved) noexcept {
    if(saved.count>kNativeMaximumStashes)return false;
    for(std::size_t i=0;i<saved.stashes.size();++i) {
        const auto& stash=saved.stashes[i];
        if(i>=saved.count) {
            for(const auto& item:stash.items)
                if(!item.empty())return false;
            continue;
        }
        if(!std::isfinite(stash.x)||!std::isfinite(stash.y)||
            stash.condition>100)return false;
        // Deployment rejects intersecting stash footprints. A corrupted
        // save must not bypass that invariant when restoring the scene.
        // The 8-pixel clearance is the same native deployment default,
        // not a claim about a special original Construct placement radius.
        for(std::size_t previous=0;previous<i;++previous) {
            const double dx=static_cast<double>(stash.x) -
                saved.stashes[previous].x;
            const double dy=static_cast<double>(stash.y) -
                saved.stashes[previous].y;
            if(dx*dx+dy*dy<16.0*16.0)return false;
        }
        for(const auto& item:stash.items) {
            if((item.item_id==0)!=(item.count==0))return false;
            if(item.empty()) {
                if(item.condition!=100 || item.fractional_loss!=0)return false;
            } else if(!valid(item))return false;
        }
    }
    stashes_=saved.stashes;
    count_=saved.count;
    return true;
}
} // namespace minidayz::items
