#pragma once
#include <array>
#include <cstddef>

namespace minidayz::items::ui {

// Nintendo 3DS lower screen: 320x240. Source sizes from the ORIGINAL
// Construct sprite frames (GUI_layout): gui_inv_card_outerwear 91/92x62,
// gui_inv_card_vest 91/92x62, gui_inv_card_backpack 216x62,
// gui_firearm 167x55 (rotated 90 degrees), gui_inventory_card
// 57x54, and gui_slot 30x30.
// Preserve each sprite's aspect ratio rather than stretching them into
// the previous seven identical 76x36 rectangles.
struct Rect {
    int x{},y{},w{},h{};
    constexpr bool contains(int px,int py) const noexcept {
        return px>=x && px<x+w && py>=y && py<y+h;
    }
};
inline constexpr std::array<Rect,8> kEquipment{{
    {8,39,56,53},     // head: source inventory card
    {68,39,92,62},    // coat: source outerwear card
    {162,39,92,62},   // vest: source vest card
    {8,103,56,34},    // pants: compact original inventory card
    {68,103,52,34},   // hands: original melee/tool slot
    {124,103,64,34},  // backpack: source card, scaled to fit
    {192,103,62,34},  // satchel: source card, scaled to fit
    {257,39,55,98}    // firearm: source gui_firearm rotated 90 degrees
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

// Tent / Supplies Stash is a different 5x3 dual-pane inventory.
// Its touch hitboxes must match the exact painted cells and ignore the
// 2px horizontal/3px vertical gutters, not round outside taps into slot 0.
struct StorageTouch {
    bool valid{};
    bool carried{};
    int local{-1};
};
constexpr StorageTouch storage_at(int px,int py) noexcept {
    if(py<99 || py>=219)return {};
    const int row=(py-99)/40;
    if(row<0 || row>=3 || (py-99)%40>=37)return {};
    for(int side=0;side<2;++side){
        const int left=side==0?8:166;
        if(px<left || px>=left+150)continue;
        const int col=(px-left)/30;
        if(col>=5 || (px-left)%30>=28)continue;
        return {true,side==0,row*5+col};
    }
    return {};
}

} // namespace minidayz::items::ui
