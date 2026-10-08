extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "gfx_table.h"
}

static C3D_Tex g_gameTex;
static C2D_Image g_gameImg;
static Tex3DS_SubTexture g_subtex;

enum GameState {
    STATE_LOADING,
    STATE_GAMEPLAY
};

// Safe upload function that adjusts subtexture dimensions to prevent GPU access crashes
void LoadSpriteByID(u32 spriteID, u16 width, u16 height) {
    if (spriteID >= TOTAL_SPRITES || ALL_SPRITES[spriteID] == NULL) return;

    // 1. Upload raw pixel data to GPU VRAM
    C3D_TexUpload(&g_gameTex, ALL_SPRITES[spriteID]);

    // 2. Adjust rendering bounds dynamically to prevent reading out-of-bounds VRAM
    g_subtex.width = width;
    g_subtex.height = height;
    g_subtex.left = 0.0f;
    g_subtex.top = 1.0f;
    g_subtex.right = (float)width / g_gameTex.width;
    g_subtex.bottom = 1.0f - ((float)height / g_gameTex.height);

    g_gameImg.tex = &g_gameTex;
    g_gameImg.subtex = &g_subtex;
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Enable console output on bottom screen for debugging
    consoleInit(GFX_BOTTOM, NULL);
    printf("Initializing 3DS Engine...\n");

    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // Initialize texture buffer (Max power-of-two size expected, e.g. 256x256)
    if (!C3D_TexInit(&g_gameTex, 256, 256, GPU_RGBA5551)) {
        printf("Failed to initialize VRAM texture buffer!\n");
    }
    C3D_TexSetFilter(&g_gameTex, GPU_NEAREST, GPU_NEAREST);

    GameState currentState = STATE_LOADING;
    u32 currentSpriteID = 0; 
    int loadingTimer = 0;

    printf("Total Sprites Loaded: %d\n", TOTAL_SPRITES);

    // Safely load index 0 on startup with explicit canvas size (e.g. 256x256 or 32x32)
    if (TOTAL_SPRITES > 0) {
        LoadSpriteByID(0, 32, 32); 
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // State Machine Logic
        if (currentState == STATE_LOADING) {
            loadingTimer++;

            if (loadingTimer >= 180 || (kDown & KEY_A)) {
                currentState = STATE_GAMEPLAY;
                printf("Switched to GAMEPLAY state.\n");
                if (TOTAL_SPRITES > 1) {
                    currentSpriteID = 1;
                    LoadSpriteByID(currentSpriteID, 32, 32);
                }
            }
        } 
        else if (currentState == STATE_GAMEPLAY) {
            if (kDown & KEY_DRIGHT) {
                currentSpriteID = (currentSpriteID + 1) % TOTAL_SPRITES;
                LoadSpriteByID(currentSpriteID, 32, 32);
                printf("Loaded Sprite ID: %lu\n", currentSpriteID);
            } else if (kDown & KEY_DLEFT) {
                currentSpriteID = (currentSpriteID == 0) ? TOTAL_SPRITES - 1 : currentSpriteID - 1;
                LoadSpriteByID(currentSpriteID, 32, 32);
                printf("Loaded Sprite ID: %lu\n", currentSpriteID);
            }
        }

        // Render Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(0x30, 0x30, 0x30, 0xFF)); // Grey background
        C2D_SceneBegin(topTarget);

        // Draw active sprite centered on top screen
        C2D_DrawImageAt(g_gameImg, 184.0f, 104.0f, 0.5f, NULL, 1.0f, 1.0f);

        C3D_FrameEnd(0);
    }

    C3D_TexDelete(&g_gameTex);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
