extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "gfx_table.h"
}

enum GameState {
    STATE_LOADING,
    STATE_GAMEPLAY
};

// Uploads sprite data to VRAM and sets full texture UV bounds to prevent texture distortion
void LoadSpriteByID(C3D_Tex* tex, Tex3DS_SubTexture* subtex, C2D_Image* img, u32 spriteID) {
    if (spriteID >= TOTAL_SPRITES || ALL_SPRITES[spriteID] == NULL) {
        printf("Error: Invalid Sprite ID %lu\n", spriteID);
        return;
    }

    // 1. Upload pixel data to GPU VRAM
    C3D_TexUpload(tex, ALL_SPRITES[spriteID]);

    // 2. Map full power-of-two texture bounds cleanly
    subtex->width = tex->width;
    subtex->height = tex->height;
    subtex->left = 0.0f;
    subtex->top = 1.0f;
    subtex->right = 1.0f;
    subtex->bottom = 0.0f;

    img->tex = tex;
    img->subtex = subtex;
}

int main(int argc, char* argv[]) {
    // 1. Initialize Hardware Services
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    printf("=== MINIDAYZ 3DS BOOT ===\n");
    printf("Initializing GPU Systems...\n");

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        printf("C3D_Init Failed!\n");
        while (aptMainLoop()) { gspWaitForVBlank(); }
    }

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        printf("C2D_Init Failed!\n");
        while (aptMainLoop()) { gspWaitForVBlank(); }
    }

    C2D_Prepare();
    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // 2. Local GPU Texture Structures
    C3D_Tex gameTex;
    C2D_Image gameImg;
    Tex3DS_SubTexture subtex;

    printf("Allocating VRAM Texture Buffer...\n");
    if (!C3D_TexInit(&gameTex, 256, 256, GPU_RGBA5551)) {
        printf("Texture Allocation Failed!\n");
    }
    C3D_TexSetFilter(&gameTex, GPU_NEAREST, GPU_NEAREST);

    printf("Total Sprites in Table: %d\n", TOTAL_SPRITES);

    GameState currentState = STATE_LOADING;
    u32 currentSpriteID = 0; 
    int loadingTimer = 0;

    // Load starting sprite
    if (TOTAL_SPRITES > 0) {
        LoadSpriteByID(&gameTex, &subtex, &gameImg, 0);
        printf("Loaded Initial Sprite 0 successfully!\n");
    }

    printf("\nEntering Loop. Press START to exit.\n");

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // --- State Machine Logic ---
        if (currentState == STATE_LOADING) {
            loadingTimer++;
            if (loadingTimer >= 180 || (kDown & KEY_A)) {
                currentState = STATE_GAMEPLAY;
                printf("State: GAMEPLAY\n");
                if (TOTAL_SPRITES > 1) {
                    currentSpriteID = 1;
                    LoadSpriteByID(&gameTex, &subtex, &gameImg, currentSpriteID);
                }
            }
        } 
        else if (currentState == STATE_GAMEPLAY) {
            if (kDown & KEY_DRIGHT) {
                currentSpriteID = (currentSpriteID + 1) % TOTAL_SPRITES;
                LoadSpriteByID(&gameTex, &subtex, &gameImg, currentSpriteID);
                printf("Sprite ID: %lu\n", currentSpriteID);
            } else if (kDown & KEY_DLEFT) {
                currentSpriteID = (currentSpriteID == 0) ? TOTAL_SPRITES - 1 : currentSpriteID - 1;
                LoadSpriteByID(&gameTex, &subtex, &gameImg, currentSpriteID);
                printf("Sprite ID: %lu\n", currentSpriteID);
            }
        }

        // --- Render Frame ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(0x00, 0x00, 0x00, 0xFF)); // Solid Black Background
        C2D_SceneBegin(topTarget);

        if (ALL_SPRITES[currentSpriteID] != NULL) {
            // Draw active sprite centered on top screen
            C2D_DrawImageAt(gameImg, 184.0f, 104.0f, 0.5f, NULL, 1.0f, 1.0f);
        }

        C3D_FrameEnd(0);
    }

    // Memory Cleanup
    C3D_TexDelete(&gameTex);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
