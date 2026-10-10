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
        // --- TOP SCREEN (Main Display) ---
        // ==========================================
        C3D_SceneBegin(topScreen);

        if (currentState == STATE_LOADING) {
            // Clear loading screen to black
            C2D_TargetClear(topScreen, C2D_Color32(0, 0, 0, 255));
            
            if (loadingSheet) {
                C2D_Image img = C2D_SpriteSheetGetImage(loadingSheet, 0);
                // Scale down the loading logo to fit comfortably on the 400x240 screen
                // 400/512 ≈ 0.78, 240/512 ≈ 0.46
                C2D_DrawImageAt(img, 0.0f, 0.0f, 0.5f, NULL, 0.75f, 0.75f);
            }
        } 
        else if (currentState == STATE_MENU) {
            // 1. Clear top screen with the signature Mini DayZ red background color
            C2D_TargetClear(topScreen, C2D_Color32(180, 25, 25, 255));

            // 2. Black City layer positioned at the bottom
            if (citySheet) {
                C2D_Image cityImg = C2D_SpriteSheetGetImage(citySheet, 0);
                // Scale and place it at the bottom of the 240p screen
                C2D_DrawImageAt(cityImg, 0.0f, 120.0f, 0.5f, NULL, 0.78f, 0.5f);
            }

            // 3. Mini DayZ Logo scaled down and centered in the middle
            if (menuSheet) {
                C2D_Image logoImg = C2D_SpriteSheetGetImage(menuSheet, 0);
                // Scaled down (0.5x) so it's not oversized, centered horizontally and vertically
                C2D_DrawImageAt(logoImg, 100.0f, 60.0f, 0.5f, NULL, 0.5f, 0.5f);
            }
        }

        // ==========================================
        // --- BOTTOM SCREEN (Left Blank) ---
        // ==========================================
        C2D_TargetClear(bottomScreen, C2D_Color32(10, 10, 10, 255));
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

