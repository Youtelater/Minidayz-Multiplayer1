extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
}

int main(int argc, char* argv[]) {
    // 1. Initialize core graphics
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // 2. Setup render targets for top and bottom screens
    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // 3. Mount romfs
    Result rc = romfsInit();

    // 4. Load spritesheet (Safely)
    C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/sprite.t3x");

    // Main render loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break; // Press START to exit back to menu

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // --- TOP SCREEN ---
        C2D_TargetClear(topTarget, C2D_Color32(0x00, 0x00, 0xFF, 0xFF)); // Solid Blue
        C2D_SceneBegin(topTarget);
        // Draw a test red square on the top screen
        C2D_DrawRectSolid(150, 70, 0, 100, 100, C2D_Color32(0xFF, 0x00, 0x00, 0xFF));

        // --- BOTTOM SCREEN ---
        C2D_TargetClear(bottomTarget, C2D_Color32(0x00, 0xFF, 0x00, 0xFF)); // Solid Green
        C2D_SceneBegin(bottomTarget);
        // Draw a test yellow square on the bottom screen
        C2D_DrawRectSolid(110, 70, 0, 100, 100, C2D_Color32(0xFF, 0xFF, 0x00, 0xFF));

        C3D_FrameEnd(0);
    }

    // Cleanup
    if (spriteSheet) C2D_SpriteSheetFree(spriteSheet);
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
