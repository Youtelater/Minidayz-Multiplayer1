#pragma once
#include <array>
#include <cstddef>

namespace minidayz::items::ui {

// Nintendo 3DS lower screen: 320x240. Source sizes from the ORIGINAL
// Construct sprite frames (GUI_layout): gui_inv_card_outerwear 91/92x62,
// gui_inv_card_vest 91/92x62, gui_inv_card_backpack 216x62,
// gui_inventory_card 57x54, and gui_slot 30x30.
// Preserve each sprite's aspect ratio rather than stretching them into
// the previous seven identical 76x36 rectangles.
struct Rect {
    int x{},y{},w{},h{};
    constexpr bool contains(int px,int py) const noexcept {
        return px>=x && px<x+w && py>=y && py<y+h;
    }
};
inline constexpr std::array<Rect,7> kEquipment{{
    {8,39,56,53},     // head: source inventory card
    {68,39,92,62},    // coat: original outerwear card, up to 92x62
    {162,39,92,62},   // vest: original vest card, up to 92x62
    {257,39,55,53},   // pants: source inventory card
    {8,103,56,34},    // hands: reflowed item slot
    {68,103,120,34},  // backpack: original 216:62 aspect
    {192,103,120,34}  // satchel: original 216:62 aspect
}};
inline constexpr int kColumns=8;
inline constexpr int kRows=2;
inline constexpr int kPageSize=15;
inline constexpr int kSlotX=24;
inline constexpr int kSlotY=157;
inline constexpr int kSlotStepX=35;
inline constexpr int kSlotStepY=35;
inline constexpr int kSlotSize=30;
inline constexpr Rect kActions{0,140,320,13};
inline constexpr Rect kPageNavigation{9,225,302,14};

constexpr int equipment_at(int x,int y) noexcept {
    for(std::size_t i=0;i<kEquipment.size();++i)
        if(kEquipment[i].contains(x,y))return static_cast<int>(i);
    return -1;
}
constexpr int inventory_at(int x,int y,std::size_t page) noexcept {
    for(int local=0;local<kPageSize;++local){
        const int col=local%kColumns, row=local/kColumns;
        const Rect box{kSlotX+col*kSlotStepX,
                       kSlotY+row*kSlotStepY,kSlotSize,kSlotSize};
        if(box.contains(x,y))
            return static_cast<int>(page*kPageSize)+local;
    }
    return -1;
}
constexpr int action_at(int x,int y) noexcept {
    return kActions.contains(x,y) ? x/64 : -1;
}

} // namespace minidayz::items::ui
