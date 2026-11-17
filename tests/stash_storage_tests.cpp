#include "minidayz/stash_storage.hpp"
#include <cassert>
#include <limits>

using namespace minidayz::items;

static bool blocked(void*,float x,float y) noexcept {
    return x<0.0f||y<0.0f;
}

int main() {
    static_assert(kOriginalSuppliesStashItemId==461);
    static_assert(kSourceSuppliesStashSlots==6);
    InventorySystem inventory;
    StashStorageSystem stash;
    assert(inventory.add(461,1,0,76));
    assert(!stash.deploy_from_inventory_checked(
        inventory,0,2,2,&blocked,nullptr,8));
    assert(inventory.count(461)==1 && stash.count()==0);
    assert(stash.deploy_from_inventory_checked(
        inventory,0,100,100,&blocked,nullptr));
    assert(stash.count()==1 && inventory.count(461)==0);
    assert(stash.stash(0)->condition==76);
    assert(inventory.add(26,3,0,100,25));
    assert(stash.store_from_inventory(inventory,0,0));
    assert(inventory.count(26)==0);
    assert(stash.stash(0)->items[0].item_id==26);
    assert(stash.stash(0)->items[0].fractional_loss==25);
    // Make a full six-stack stash and verify no data is consumed by a
    // rejected transfer.
    for(unsigned i=0;i<5;++i) {
        assert(inventory.add_to_slot(0,static_cast<std::uint16_t>(10+i),1));
        assert(stash.store_from_inventory(inventory,0,0));
    }
    assert(inventory.add_to_slot(0,23,1));
    assert(!stash.store_from_inventory(inventory,0,0));
    assert(inventory.slot(0)->item_id==23);
    assert(stash.stash(0)->items[5].item_id==14);
    assert(inventory.remove(0,1));
    assert(stash.take_to_inventory(inventory,0,0));
    assert(inventory.count(26)==3);
    assert(inventory.slot(0)->condition==100);
    assert(inventory.slot(0)->fractional_loss==25);
    auto saved=stash.snapshot();
    StashStorageSystem restored;
    assert(restored.restore(saved));
    assert(restored.stash(0)->items[0].empty());
    assert(restored.stash(0)->items[1].item_id==10);
    saved.stashes[0].items[1].fractional_loss=255;
    assert(!restored.restore(saved));
    assert(restored.stash(0)->items[1].item_id==10);
    assert(stash.nearest(100,100,3)==0);
    assert(stash.nearest(200,200,3)==stash.count());
    assert(!stash.deploy_from_inventory_checked(
        inventory,0,std::numeric_limits<float>::quiet_NaN(),0,
        nullptr,nullptr));
    return 0;
}
