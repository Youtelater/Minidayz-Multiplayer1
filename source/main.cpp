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

    // Load texture sheets from RomFS
    C2D_SpriteSheet loadingSheet = C2D_SpriteSheetLoad("romfs:/loading_logo.t3x");
    C2D_SpriteSheet menuSheet    = C2D_SpriteSheetLoad("romfs:/main_menu_logo_sheet0.t3x");
    C2D_SpriteSheet citySheet    = C2D_SpriteSheetLoad("romfs:/tiledbgmenu_city_up.t3x");

    GameState currentState = STATE_LOADING;
    int loadingTimer = 0;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            break;
        }

        // State logic: Auto-switch from loading to menu after ~2 seconds or press A
        if (currentState == STATE_LOADING) {
            loadingTimer++;
            if (loadingTimer > 120 || (kDown & KEY_A)) {
                currentState = STATE_MENU;
            }
        }

        // Render frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // ==========================================
        // 1. TOP SCREEN (Loading & Menu Layout)
        // ==========================================
        C2D_SceneBegin(topScreen);

        if (currentState == STATE_LOADING) {
            C2D_TargetClear(topScreen, C2D_Color32(0, 0, 0, 255));
            if (loadingSheet) {
                C2D_Image img = C2D_SpriteSheetGetImage(loadingSheet, 0);
                // Center the loading logo on the 400x240 screen
                C2D_DrawImageAt(img, 75.0f, 40.0f, 0.5f, NULL, 0.65f, 0.65f);
            }
        } 
        else if (currentState == STATE_MENU) {
            // Signature Mini DayZ Red Background
            C2D_TargetClear(topScreen, C2D_Color32(180, 25, 25, 255));

            // Black City layer anchored cleanly to the bottom of the 240p screen
            if (citySheet) {
                C2D_Image cityImg = C2D_SpriteSheetGetImage(citySheet, 0);
                C2D_DrawImageAt(cityImg, 0.0f, 150.0f, 0.5f, NULL, 0.78f, 0.45f);
            }

            // Game Logo centered in the middle of the screen
            if (menuSheet) {
                C2D_Image logoImg = C2D_SpriteSheetGetImage(menuSheet, 0);
                C2D_DrawImageAt(logoImg, 85.0f, 75.0f, 0.5f, NULL, 0.55f, 0.55f);
            }
        }

        // ==========================================
        // 2. BOTTOM SCREEN (Reserved for future UI)
        // ==========================================
        C2D_TargetClear(bottomScreen, C2D_Color32(0, 0, 0, 255));
        C2D_SceneBegin(bottomScreen);

        C3D_FrameEnd(0);
    }

    // Cleanup
    if (loadingSheet) C2D_SpriteSheetFree(loadingSheet);
    if (menuSheet)   C2D_SpriteSheetFree(menuSheet);
    if (citySheet)   C2D_SpriteSheetFree(citySheet);
    
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
