#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Initialize graphics systems
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Set up console on bottom screen
    PrintConsole bottomConsole;
    consoleInit(GFX_BOTTOM, &bottomConsole);

    printf("--- MINIDAYZ RUNTIME DEBUG ---\n\n");

    // 1. Check RomFS Initialization
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        printf("[FAIL] romfsInit() failed: %08lX\n", rc);
    } else {
        printf("[OK] RomFS mounted successfully!\n");
    }

    C2D_Target* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // 2. Load Sprite Sheet from RomFS
    C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");

    if (spriteSheet == NULL) {
        printf("[FAIL] Failed to load romfs:/gfx/sprites.t3x\n");
    } else {
        size_t count = C2D_SpriteSheetCount(spriteSheet);
        printf("[OK] Loaded sprites.t3x! Total sprites: %zu\n", count);
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(0x30, 0x60, 0x30, 0xFF));
        C2D_SceneBegin(topTarget);

        if (spriteSheet != NULL) {
            // Draw the first sprite in the atlas (index 0)
            C2D_Image playerSprite = C2D_SpriteSheetGetImage(spriteSheet, 0);
            C2D_DrawImageAt(playerSprite, 180.0f, 100.0f, 0.5f, NULL, 1.0f, 1.0f);
        } else {
            // Fallback red square indicator
            C2D_DrawRectSolid(192.0f, 112.0f, 0.5f, 16.0f, 16.0f, C2D_Color32(255, 0, 0, 255));
        }

        C3D_FrameEnd(0);
    }

    // Clean up resources
    if (spriteSheet) C2D_SpriteSheetFree(spriteSheet);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
