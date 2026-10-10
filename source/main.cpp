#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <tex3ds.h>
#include <stdlib.h>

enum GameState {
    STATE_LOADING,
    STATE_MENU
};

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    romfsInit();

    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Load the loading logo and the menu background/logo sheets
    C2D_SpriteSheet loadingSheet = C2D_SpriteSheetLoad("romfs:/loading_logo.t3x");
    C2D_SpriteSheet menuSheet    = C2D_SpriteSheetLoad("romfs:/main_menu_logo_sheet0.t3x");
    C2D_SpriteSheet citySheet    = C2D_SpriteSheetLoad("romfs:/tiledbgmenu_city_up.t3x");
    C2D_SpriteSheet skySheet     = C2D_SpriteSheetLoad("romfs:/tiledbgmenu_sky_sprite_sheet0.t3x");

    GameState currentState = STATE_LOADING;
    int loadingTimer = 0;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            break;
        }

        // State transition logic
        if (currentState == STATE_LOADING) {
            loadingTimer++;
            // Switch from Loading to Menu after a short timer or pressing A
            if (loadingTimer > 120 || (kDown & KEY_A)) {
                currentState = STATE_MENU;
            }
        }

        // Render frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(topScreen, C2D_Color32(0, 0, 0, 255));
        C2D_SceneBegin(topScreen);

        C2D_TargetClear(bottomScreen, C2D_Color32(20, 20, 20, 255));
        C2D_SceneBegin(bottomScreen);

        if (currentState == STATE_LOADING) {
            // Draw loading screen logo
            if (loadingSheet) {
                C2D_Image img = C2D_SpriteSheetGetImage(loadingSheet, 0);
                C2D_DrawImageAt(img, 50.0f, 50.0f, 0.5f, NULL, 1.0f, 1.0f);
            }
        } 
        else if (currentState == STATE_MENU) {
            // 1. Draw Red Background / Sky layer first
            if (skySheet) {
                C2D_Image skyImg = C2D_SpriteSheetGetImage(skySheet, 0);
                C2D_DrawImageAt(skyImg, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
            }

            // 2. Draw Black City layer on the bottom
            if (citySheet) {
                C2D_Image cityImg = C2D_SpriteSheetGetImage(citySheet, 0);
                // Adjust Y coordinate if needed so it sits properly at the bottom of the screen
                C2D_DrawImageAt(cityImg, 0.0f, 120.0f, 0.5f, NULL, 1.0f, 1.0f);
            }

            // 3. Draw Logo in the middle
            if (menuSheet) {
                C2D_Image logoImg = C2D_SpriteSheetGetImage(menuSheet, 0);
                // Adjust coordinates to center it on the top screen
                C2D_DrawImageAt(logoImg, 80.0f, 60.0f, 0.5f, NULL, 1.0f, 1.0f);
            }
        }

        C3D_FrameEnd(0);
    }

    // Cleanup all loaded sheets
    if (loadingSheet) C2D_SpriteSheetFree(loadingSheet);
    if (menuSheet)   C2D_SpriteSheetFree(menuSheet);
    if (citySheet)   C2D_SpriteSheetFree(citySheet);
    if (skySheet)    C2D_SpriteSheetFree(skySheet);
    
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
