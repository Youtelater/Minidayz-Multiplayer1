#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

// Screen dimensions
#define TOP_WIDTH   400
#define TOP_HEIGHT  240
#define BOT_WIDTH   320
#define BOT_HEIGHT  240

struct InventorySlot {
    int x, y, width, height;
    int itemID;
};

// UI Slot positions for bottom screen
InventorySlot slotShirt    = { 40,  30, 60, 60, 1 };
InventorySlot slotPants    = { 40, 110, 60, 60, 2 };
InventorySlot slotBackpack = { 120, 30, 60, 60, 0 };
InventorySlot slotRifle    = { 200, 30, 60, 140, 3 };

int selectedSlotID = -1;

bool isTouched(InventorySlot slot, u16 touchX, u16 touchY) {
    return (touchX >= slot.x && touchX <= (slot.x + slot.width) &&
            touchY >= slot.y && touchY <= (slot.y + slot.height));
}

int main(int argc, char **argv) {
    // 1. Initialize default 3DS graphics system
    gfxInitDefault();
    
    // Enable double buffering on both screens explicitly
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    // 2. Initialize Citro3D and Citro2D GPU frameworks
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 0;
    }

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        gfxExit();
        return 0;
    }

    C2D_Prepare();

    // 3. Create render targets for both screens
    C3D_RenderTarget* topTarget    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Color definitions
    u32 clrGrass     = C2D_Color32(76, 115, 61, 255);   // Gameplay background (Top)
    u32 clrUIBg      = C2D_Color32(35, 30, 25, 255);   // Inventory background (Bottom)
    u32 clrSlotFill  = C2D_Color32(180, 170, 150, 255); // Item slot box
    u32 clrSelected  = C2D_Color32(80, 200, 80, 255);   // Selection highlight

    // Main Game Loop
    while (aptMainLoop()) {
        // --- INPUT SCANNING ---
        hidScanInput();
        u32 kDown = hidKeysDown();

        touchPosition touch;
        hidTouchRead(&touch);

        // Exit on START button
        if (kDown & KEY_START) break;

        // --- TOUCH INPUT DETECTION ---
        if (kDown & KEY_TOUCH) {
            if (isTouched(slotShirt, touch.px, touch.py)) {
                selectedSlotID = 1;
            } else if (isTouched(slotPants, touch.px, touch.py)) {
                selectedSlotID = 2;
            } else if (isTouched(slotRifle, touch.px, touch.py)) {
                selectedSlotID = 3;
            } else if (isTouched(slotBackpack, touch.px, touch.py)) {
                selectedSlotID = 0;
            }
        }

        // --- RENDER FRAME ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // A. RENDER TOP SCREEN
        C2D_TargetClear(topTarget, clrGrass);
        C2D_SceneBegin(topTarget);
        
        // Draw Red Player Box in center of top screen
        C2D_DrawRectangle(190, 110, 0, 20, 20, 
                           C2D_Color32(200, 50, 50, 255), 
                           C2D_Color32(200, 50, 50, 255), 
                           C2D_Color32(200, 50, 50, 255), 
                           C2D_Color32(200, 50, 50, 255));

        // B. RENDER BOTTOM SCREEN
        C2D_TargetClear(bottomTarget, clrUIBg);
        C2D_SceneBegin(bottomTarget);

        // Helper to render inventory slots
        auto drawSlotUI = [&](InventorySlot slot) {
            u32 borderClr = (selectedSlotID == slot.itemID && slot.itemID != 0) ? clrSelected : clrSlotFill;
            C2D_DrawRectangle(slot.x, slot.y, 0, slot.width, slot.height, 
                               borderClr, borderClr, borderClr, borderClr);
        };

        drawSlotUI(slotShirt);
        drawSlotUI(slotPants);
        drawSlotUI(slotBackpack);
        drawSlotUI(slotRifle);

        C3D_FrameEnd(0);
    }

    // Deinitialize graphics
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
