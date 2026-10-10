#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include "gfx_table.h" // Your auto-generated header

int main(int argc, char* argv[]) {
    // 1. Initialize core 3DS services
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // 2. Initialize RomFS so it can read files from 'romfs:/'
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        // Handle initialization failure if necessary
    }

    // 3. Load textures from RomFS using the normalized asset names
    C2D_SpriteSheet loadingSheet = C2D_SpriteSheetLoad("romfs:/gfx/loading_logo.t3x");
    C2D_SpriteSheet menuSheet = C2D_SpriteSheetLoad("romfs:/gfx/main_menu_logo_sheet0.t3x");

    C2D_Sprite loadingSprite;
    C2D_Sprite menuSprite;

    if (loadingSheet) {
        C2D_SpriteFromSheet(&loadingSprite, loadingSheet, 0);
        C2D_SpriteSetCenter(&loadingSprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&loadingSprite, 200.0f, 120.0f); // Center of top screen
    }

    if (menuSheet) {
        C2D_SpriteFromSheet(&menuSprite, menuSheet, 0);
        C2D_SpriteSetCenter(&menuSprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&menuSprite, 200.0f, 120.0f);
    }

    bool showMenu = false;

    // 4. Main game loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break; // Press Start to exit

        // Press A to toggle between loading logo and menu logo
        if (kDown & KEY_A) {
            showMenu = !showMenu;
        }

        // --- RENDER FRAME ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(C2D_GetScreenTop(), C2D_Color32(30, 30, 30, 255));
        
        C2D_SceneBegin(C2D_GetScreenTop());
        if (!showMenu && loadingSheet) {
            C2D_DrawSprite(&loadingSprite);
        } else if (showMenu && menuSheet) {
            C2D_DrawSprite(&menuSprite);
        }

        C2D_Flush();
        C3D_FrameEnd(0);
    }

    // Clean up resources before exiting
    if (loadingSheet) C2D_SpriteSheetFree(loadingSheet);
    if (menuSheet) C2D_SpriteSheetFree(menuSheet);

    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
