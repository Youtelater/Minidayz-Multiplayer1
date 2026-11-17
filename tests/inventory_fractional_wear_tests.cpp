#include "minidayz/inventory.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/tent_storage.hpp"
#include <cassert>

using namespace minidayz::items;

int main() {
    InventorySystem inventory;
    GroundLootSystem ground;
    assert(inventory.add(244, 1, 0, 100));
    // Firearm Craft source requires all guards (including the held item
    // identity) before dispatch. Invalid actions must remain no-ops.
    assert(!inventory.apply_verified_wear_action(0,
        6904772320561696ULL,875054325568454ULL,744011060201717ULL,
        false,false));
    assert(inventory.slot(0)->fractional_loss == 0);
    for(unsigned i=0;i<3;++i)
        assert(inventory.apply_verified_wear_action(0,
            6904772320561696ULL,875054325568454ULL,744011060201717ULL,
            true,false));
    assert(inventory.slot(0)->condition == 100);
    assert(inventory.slot(0)->fractional_loss == 75);
    assert(!inventory.apply_verified_wear_action(0,
        6904772320561696ULL,875054325568454ULL,744011060201717ULL,
        true,true)); // perk_wxnj cancels the verified wear expression
    assert(inventory.slot(0)->fractional_loss == 75);
    assert(inventory.move(0,4));
    assert(inventory.slot(4)->fractional_loss == 75);
    assert(ground.drop_from_inventory(inventory, 4, 1, 20, 40));
    assert(ground.entry(0)->item.condition == 100);
    assert(ground.entry(0)->item.fractional_loss == 75);
    assert(ground.pickup_nearest(inventory, 20, 40, 3));
    const auto* item=inventory.slot(0);
    assert(item && item->item_id == 244 && item->fractional_loss == 75);
    assert(inventory.apply_verified_wear_action(0,
        6904772320561696ULL,875054325568454ULL,744011060201717ULL,
        true,false));
    item=inventory.slot(0);
    assert(item->condition == 99 && item->fractional_loss == 0);
    auto snap=inventory.snapshot();
    InventorySystem restored;
    assert(restored.restore(snap));
    assert(restored.slot(0)->item_id == 244);
    assert(restored.slot(0)->condition == 99);
    assert(restored.damage_condition_hundredths(0,25));
    assert(restored.slot(0)->condition == 99);
    assert(restored.slot(0)->fractional_loss == 25);
    assert(!restored.add_to_slot(0,244,1,0,99,0));
    assert(restored.add_to_slot(1,244,1,0,99,0));
    assert(restored.slot(1)->fractional_loss == 0);
    assert(restored.restore_condition(0,100));
    assert(restored.slot(0)->fractional_loss == 0);

    // A storage transfer must not round away sub-percent item wear.
    assert(restored.add(456,1));
    std::size_t tent_slot=restored.capacity();
    for(std::size_t i=0;i<restored.capacity();++i)
        if(restored.slot(i)->item_id==456){tent_slot=i;break;}
    assert(tent_slot<restored.capacity());
    TentStorageSystem tents;
    assert(tents.deploy_from_inventory(restored,tent_slot,50,55));
    assert(restored.damage_condition_hundredths(1,50));
    assert(tents.store_from_inventory(restored,1,0));
    assert(tents.tent(0)->items[0].fractional_loss==50);
    assert(tents.take_to_inventory(restored,0,0));
    bool preserved=false;
    for(std::size_t i=0;i<restored.capacity();++i) {
        const auto* s=restored.slot(i);
        if(s->item_id==244 && s->fractional_loss==50) preserved=true;
    }
    assert(preserved);
    return 0;
}
