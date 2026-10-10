#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <tex3ds.h>
#include <stdlib.h>
#include <stdio.h>

enum GameState {
    STATE_LOADING,
    STATE_MENU
};

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Initialize console on the bottom screen for debugging error messages
    PrintConsole bottomConsole;
    consoleInit(GFX_BOTTOM, &bottomConsole);

    printf("\nInitializing RomFS...\n");
    Result romfsRes = romfsInit();
    if (romfsRes != 0) {
        printf("ERROR: romfsInit failed: 0x%08lX\n", romfsRes);
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 0;
    }
    printf("RomFS OK!\n");

    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Load sheets and check for failures
    printf("Loading textures...\n");
    C2D_SpriteSheet loadingSheet = C2D_SpriteSheetLoad("romfs:/loading_logo.t3x");
    C2D_SpriteSheet menuSheet    = C2D_SpriteSheetLoad("romfs:/main_menu_logo_sheet0.t3x");
    C2D_SpriteSheet citySheet    = C2D_SpriteSheetLoad("romfs:/tiledbgmenu_city_up.t3x");
    C2D_SpriteSheet skySheet     = C2D_SpriteSheetLoad("romfs:/tiledbgmenu_sky_sprite_sheet0.t3x");

    if (!loadingSheet) printf("Warning: loading_logo.t3x failed to load\n");
    if (!menuSheet)    printf("Warning: main_menu_logo_sheet0.t3x failed\n");
    if (!citySheet)    printf("Warning: tiledbgmenu_city_up.t3x failed\n");
    if (!skySheet)     printf("Warning: tiledbgmenu_sky_sprite_sheet0.t3x failed\n");

    GameState currentState = STATE_LOADING;
    int loadingTimer = 0;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            break;
        }

        if (currentState == STATE_LOADING) {
            loadingTimer++;
            if (loadingTimer > 120 || (kDown & KEY_A)) {
                currentState = STATE_MENU;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(topScreen, C2D_Color32(30, 30, 30, 255));
        C2D_SceneBegin(topScreen);

        if (currentState == STATE_LOADING) {
            if (loadingSheet) {
                C2D_Image img = C2D_SpriteSheetGetImage(loadingSheet, 0);
                C2D_DrawImageAt(img, 50.0f, 50.0f, 0.5f, NULL, 1.0f, 1.0f);
            }
        } 
        else if (currentState == STATE_MENU) {
            if (skySheet) {
                C2D_Image skyImg = C2D_SpriteSheetGetImage(skySheet, 0);
                C2D_DrawImageAt(skyImg, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
            }
            if (citySheet) {
                C2D_Image cityImg = C2D_SpriteSheetGetImage(citySheet, 0);
                C2D_DrawImageAt(cityImg, 0.0f, 120.0f, 0.5f, NULL, 1.0f, 1.0f);
            }
            if (menuSheet) {
                C2D_Image logoImg = C2D_SpriteSheetGetImage(menuSheet, 0);
                C2D_DrawImageAt(logoImg, 80.0f, 60.0f, 0.5f, NULL, 1.0f, 1.0f);
            }
        }

        C3D_FrameEnd(0);
    }

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
