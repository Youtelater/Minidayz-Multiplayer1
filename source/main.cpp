#include <citro2d.h>
#include <3ds.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Initialize standard services
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create top screen render target
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // Load your converted graphic from RomFS
    // (Note: hyphens in filenames become underscores due to our pipeline)
    C2D_SpriteSheet loadingLogoSheet = C2D_SpriteSheetLoad("romfs:/gfx/loading_logo.t3x");
    
    C2D_Sprite logoSprite;
    if (loadingLogoSheet) {
        C2D_SpriteFromSheet(&logoSprite, loadingLogoSheet, 0);
        C2D_SpriteSetCenter(&logoSprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&logoSprite, 200.0f, 120.0f); // Center on top screen
    }

    // Main game loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break; // Exit on START

        // Render the scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetBegin(topScreen);
        C2D_SceneClear(topScreen, C2D_Color32(0, 0, 0, 255)); // Black background

        // Draw the logo if loaded successfully
        if (loadingLogoSheet) {
            C2D_DrawSprite(&logoSprite);
        }

        C2D_TargetEnd();
        C3D_FrameEnd(0);
    }

    // Clean up
    if (loadingLogoSheet) C2D_SpriteSheetFree(loadingLogoSheet);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}
