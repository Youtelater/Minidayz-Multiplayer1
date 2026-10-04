extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
}

// Grid layout settings for bottom screen inventory
const int GRID_ROWS = 3;
const int GRID_COLS = 5;
const int SLOT_SIZE = 40;
const int START_X = 35;
const int START_Y = 40;

// Item IDs matching sprite sheet order (0 = first image, 1 = second image, etc.)
enum ItemID {
    EMPTY = -1,
    ITEM_0 = 0,
    ITEM_1 = 1,
    ITEM_2 = 2
};

// 15 inventory slots (matches 3 rows x 5 columns)
int inventory[15] = {
    ITEM_0, ITEM_1, ITEM_2, EMPTY, EMPTY,
    EMPTY,  ITEM_1, EMPTY,  EMPTY, EMPTY,
    EMPTY,  EMPTY,  EMPTY,  EMPTY, EMPTY
};

int main(int argc, char* argv[]) {
    // 1. Initialize System Libraries
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // 2. Mount romfs filesystem
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        gfxExit();
        return 0;
    }

    // 3. Set up render targets for Top (400x240) and Bottom (320x240) screens
    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // 4. Load Spritesheet (Must be placed in romfs/sprite.t3x)
    C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/sprite.t3x");

    int selectedSlot = 0;
    touchPosition touch;

    // --- Main Game Loop ---
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break; // Exit application

        // Read touch input on the bottom screen
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);

            for (int r = 0; r < GRID_ROWS; r++) {
                for (int c = 0; c < GRID_COLS; c++) {
                    int slotX = START_X + (c * (SLOT_SIZE + 10));
                    int slotY = START_Y + (r * (SLOT_SIZE + 10));

                    if (touch.px >= slotX && touch.px <= slotX + SLOT_SIZE &&
                        touch.py >= slotY && touch.py <= slotY + SLOT_SIZE) {
                        selectedSlot = r * GRID_COLS + c;
                    }
                }
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // ==========================================
        // TOP SCREEN (Gameplay View)
        // ==========================================
        C2D_TargetClear(topTarget, C2D_Color32(0x68, 0xB0, 0xD8, 0xFF));
        C2D_SceneBegin(topTarget);

        // Draw active selected item on the top screen display
        if (spriteSheet && inventory[selectedSlot] != EMPTY) {
            C2D_Image selectedImg = C2D_SpriteSheetGetImage(spriteSheet, inventory[selectedSlot]);
            C2D_DrawImageAt(selectedImg, 180.0f, 100.0f, 0.5f, NULL, 1.0f, 1.0f);
        }

        // ==========================================
        // BOTTOM SCREEN (Inventory View)
        // ==========================================
        C2D_TargetClear(bottomTarget, C2D_Color32(0x20, 0x20, 0x20, 0xFF));
        C2D_SceneBegin(bottomTarget);

        // Render Inventory Grid
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                int slotIndex = r * GRID_COLS + c;
                int x = START_X + (c * (SLOT_SIZE + 10));
                int y = START_Y + (r * (SLOT_SIZE + 10));

                u32 borderCol = (slotIndex == selectedSlot)
                    ? C2D_Color32(0xFF, 0xD7, 0x00, 0xFF)   // Gold highlight
                    : C2D_Color32(0x50, 0x50, 0x50, 0xFF);  // Gray border

                // Draw Slot Background
                C2D_DrawRectSolid(x - 2, y - 2, 0, SLOT_SIZE + 4, SLOT_SIZE + 4, borderCol);
                C2D_DrawRectSolid(x, y, 0, SLOT_SIZE, SLOT_SIZE, C2D_Color32(0x10, 0x10, 0x10, 0xFF));

                // Draw Item Image in Slot
                int itemID = inventory[slotIndex];
                if (itemID != EMPTY && spriteSheet) {
                    C2D_Image itemImg = C2D_SpriteSheetGetImage(spriteSheet, itemID);
                    C2D_DrawImageAt(itemImg, x + 4.0f, y + 4.0f, 0.5f, NULL, 1.0f, 1.0f);
                }
            }
        }

        C3D_FrameEnd(0);
    }

    // Cleanup assets
    if (spriteSheet) C2D_SpriteSheetFree(spriteSheet);
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}