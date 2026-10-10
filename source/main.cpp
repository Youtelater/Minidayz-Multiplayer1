#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <tex3ds.h>

// ... rest of your code


enum GameState {
    STATE_LOADING = 0,
    STATE_MAIN_MENU
};

int main(int argc, char* argv[]) {
    // Initialize standard services
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create target for top screen
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // --- LOAD ASSETS ---
    // 1. Loading Logo
    C2D_SpriteSheet loadingSheet = C2D_SpriteSheetLoad("romfs:/gfx/loading_logo.t3x");
    C2D_Sprite loadingSprite;
    if (loadingSheet) {
        C2D_SpriteFromSheet(&loadingSprite, loadingSheet, 0);
        C2D_SpriteSetCenter(&loadingSprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&loadingSprite, 400.0f / 2.0f, 240.0f / 2.0f);
    }

    // 2. Main Menu Assets
    C2D_SpriteSheet skySheet = C2D_SpriteSheetLoad("romfs:/gfx/tiledbgmenu_sky_sprite_sheet0.t3x");
    C2D_SpriteSheet citySheet = C2D_SpriteSheetLoad("romfs:/gfx/tiledbgmenu_city_up.t3x");
    C2D_SpriteSheet menuLogoSheet = C2D_SpriteSheetLoad("romfs:/gfx/main_menu_logo_sheet0.t3x");

    C2D_Sprite skySprite, citySprite, logoSprite;
    if (skySheet)   C2D_SpriteFromSheet(&skySprite, skySheet, 0);
    if (citySheet)  C2D_SpriteFromSheet(&citySprite, citySheet, 0);
    if (menuLogoSheet) C2D_SpriteFromSheet(&logoSprite, menuLogoSheet, 0);

    // Position menu elements
    if (menuLogoSheet) {
        C2D_SpriteSetCenter(&logoSprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&logoSprite, 400.0f / 2.0f, 90.0f); // Positioned nicely in the upper-center
    }
    if (citySheet) {
        C2D_SpriteSetCenter(&citySprite, 0.5f, 1.0f);
        C2D_SpriteSetPos(&citySprite, 400.0f / 2.0f, 240.0f); // Anchored to the bottom of the top screen
    }

    GameState currentState = STATE_LOADING;
    int frameCounter = 0;

    // Main application loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break; // Exit on START

        // --- STATE LOGIC ---
        if (currentState == STATE_LOADING) {
            frameCounter++;
            // Automatically transition to main menu after ~150 frames or pressing A
            if (frameCounter > 150 || (kDown & KEY_A)) {
                if (loadingSheet) {
                    C2D_SpriteSheetFree(loadingSheet);
                    loadingSheet = NULL;
                }
                currentState = STATE_MAIN_MENU;
            }
        }

        // --- RENDERING ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetBegin(topScreen);
        C2D_SceneClear(topScreen, C2D_Color32(0, 0, 0, 255));

        if (currentState == STATE_LOADING && loadingSheet) {
            C2D_DrawSprite(&loadingSprite);
        } 
        else if (currentState == STATE_MAIN_MENU) {
            // 1. Draw Red Sky Background
            if (skySheet) {
                C2D_DrawSprite(&skySprite);
            }

            // 2. Draw City Skyline at the bottom
            if (citySheet) {
                C2D_DrawSprite(&citySprite);
            }

            // 3. Draw Mini DAYZ+ Title Logo over the top
            if (menuLogoSheet) {
                C2D_DrawSprite(&logoSprite);
            }
        }

        C2D_TargetEnd();
        C3D_FrameEnd(0);
    }

    // Clean up memory
    if (loadingSheet) C2D_SpriteSheetFree(loadingSheet);
    if (skySheet) C2D_SpriteSheetFree(skySheet);
    if (citySheet) C2D_SpriteSheetFree(citySheet);
    if (menuLogoSheet) C2D_SpriteSheetFree(menuLogoSheet);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    
    return 0;
}
