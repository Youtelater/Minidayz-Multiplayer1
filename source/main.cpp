#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Initialize graphics systems
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Set up console on bottom screen for real-time debugging
    PrintConsole bottomConsole;
    consoleInit(GFX_BOTTOM, &bottomConsole);

    printf("--- MINIDAYZ RUNTIME DEBUG ---\n\n");

    // Initialize RomFS
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        printf("[FAIL] romfsInit() failed: %08lX\n", (unsigned long)rc);
    } else {
        printf("[OK] RomFS mounted successfully!\n");
    }

    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");

    if (spriteSheet == NULL) {
        printf("[FAIL] Failed to load romfs:/gfx/sprites.t3x\n");
    } else {
        printf("[OK] Loaded sprites.t3x!\n");
    }

    // Player position coordinates
    float playerX = 184.0f;
    float playerY = 104.0f;
    float moveSpeed = 2.0f;

    // Sprite frame dimensions (adjust if your grid frames are a different size, e.g., 32x32)
    float frameWidth = 32.0f;
    float frameHeight = 32.0f;

    // Crop offsets for player.png grid
    float frameX = 0.0f;
    float frameY = 0.0f;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kHeld = hidKeysHeld();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // Directional input & row/column selection from player.png grid
        if (kHeld & (KEY_DUP | KEY_CPAD_UP)) {
            playerY -= moveSpeed;
            frameY = frameHeight * 1.0f; // Row 2 (Facing Up)
        }
        else if (kHeld & (KEY_DDOWN | KEY_CPAD_DOWN)) {
            playerY += moveSpeed;
            frameY = frameHeight * 0.0f; // Row 1 (Facing Down)
        }

        if (kHeld & (KEY_DLEFT | KEY_CPAD_LEFT)) {
            playerX -= moveSpeed;
            frameY = frameHeight * 2.0f; // Row 3 (Facing Left)
        }
        else if (kHeld & (KEY_DRIGHT | KEY_CPAD_RIGHT)) {
            playerX += moveSpeed;
            frameY = frameHeight * 3.0f; // Row 4 (Facing Right)
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(0x30, 0x60, 0x30, 0xFF));
        C2D_SceneBegin(topTarget);

        if (spriteSheet != NULL) {
            C2D_Image fullSheet = C2D_SpriteSheetGetImage(spriteSheet, 0);

            // Fetch texture dimensions dynamically from texture object
            float texW = (float)fullSheet.tex->width;
            float texH = (float)fullSheet.tex->height;

            // Dynamically cut out the active frame coordinates from player.png
            Tex3DS_SubTexture frameSubTex = {
                (u16)frameWidth, (u16)frameHeight,
                frameX / texW,                        // Left UV boundary
                (frameY + frameHeight) / texH,       // Top UV boundary
                (frameX + frameWidth) / texW,        // Right UV boundary
                frameY / texH                        // Bottom UV boundary
            };

            C2D_Image playerFrame = { fullSheet.tex, &frameSubTex };
            C2D_DrawImageAt(playerFrame, playerX, playerY, 0.5f, NULL, 1.0f, 1.0f);
        } else {
            // Fallback square if texture is missing
            C2D_DrawRectSolid(playerX, playerY, 0.5f, 16.0f, 16.0f, C2D_Color32(255, 0, 0, 255));
        }

        C3D_FrameEnd(0);
    }

    if (spriteSheet) C2D_SpriteSheetFree(spriteSheet);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
