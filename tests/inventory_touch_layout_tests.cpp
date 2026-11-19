#include "minidayz/inventory_touch_layout.hpp"
#include <cassert>

int main(){
    using namespace minidayz::items::ui;
    static_assert(kPageSize==15);
    static_assert(kSlotSize==30);
    static_assert(kColumns==8 && kRows==2);
    // All original equipment hitboxes are independent and match the
    // parchment artwork used by source/main.cpp on the 320x240 touch UI.
    for(int i=0;i<static_cast<int>(kEquipment.size());++i){
        const auto& r=kEquipment[static_cast<std::size_t>(i)];
        assert(equipment_at(r.x+r.w/2,r.y+r.h/2)==i);
        assert(r.x>=0 && r.y>=0 && r.x+r.w<=320 && r.y+r.h<140);
        for(int j=i+1;j<static_cast<int>(kEquipment.size());++j){
            const auto& b=kEquipment[static_cast<std::size_t>(j)];
            const bool overlap=r.x<b.x+b.w && b.x<r.x+r.w &&
                               r.y<b.y+b.h && b.y<r.y+r.h;
            assert(!overlap);
        }
    }
    // The old layout squashed the original square gui_slot artwork
    // into 56x29 stretched horizontal tiles. New geometry is 30x30.
    for(int page=0;page<3;++page){
        for(int local=0;local<kPageSize;++local){
            const int cx=kSlotX+(local%kColumns)*kSlotStepX+15;
            const int cy=kSlotY+(local/kColumns)*kSlotStepY+15;
            assert(inventory_at(cx,cy,page)==page*kPageSize+local);
            assert(equipment_at(cx,cy)==-1);
        }
    }
    assert(inventory_at(0,0,0)==-1);
    assert(inventory_at(305,222,0)==-1); // unused 16th tile
    assert(inventory_at(300,238,0)==-1);
    assert(kActions.y>=137 && kActions.y+kActions.h<=kSlotY);
    assert(action_at(32,146)==0);
    assert(action_at(96,146)==1);
    assert(action_at(160,146)==2);
    assert(action_at(224,146)==3);
    assert(action_at(288,146)==4);
    assert(action_at(160,160)==-1);
    assert(kPageNavigation.contains(12,230));
    assert(!kPageNavigation.contains(30,215));
}
