extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "gfx_table.h"
}

// Keep all heavy structures static/global to prevent STACK OVERFLOW
static C3D_Tex g_gameTex;
static C2D_Image g_gameImg;
static Tex3DS_SubTexture g_subtex;

enum GameState {
    STATE_LOADING,
    STATE_GAMEPLAY
};

void LoadSpriteByID(u32 spriteID, u16 width, u16 height) {
    if (spriteID >= TOTAL_SPRITES || ALL_SPRITES[spriteID] == NULL) {
        printf("Error: Invalid Sprite ID %lu\n", spriteID);
        return;
    }

    // Upload raw pixel data to GPU VRAM
    C3D_TexUpload(&g_gameTex, ALL_SPRITES[spriteID]);

    // Setup Subtexture bounds dynamically
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
    // 1. MUST INITIALIZE GRAPHICS AND CONSOLE FIRST
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    printf("=== MINIDAYZ 3DS BOOT ===\n");
    printf("Initializing Citro3D & Citro2D...\n");

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

    printf("Allocating VRAM Texture Buffer...\n");
    // Allocate 256x256 buffer in VRAM
    if (!C3D_TexInit(&g_gameTex, 256, 256, GPU_RGBA5551)) {
        printf("Texture Allocation Failed!\n");
    }
    C3D_TexSetFilter(&g_gameTex, GPU_NEAREST, GPU_NEAREST);

    printf("Total Sprites in Table: %d\n", TOTAL_SPRITES);

    GameState currentState = STATE_LOADING;
    u32 currentSpriteID = 0; 
    int loadingTimer = 0;

    // Load first sprite on boot
    if (TOTAL_SPRITES > 0) {
        LoadSpriteByID(0, 32, 32);
        printf("Loaded Initial Sprite 0 successfully!\n");
    }

    printf("Entering Main Loop. Press START to exit.\n");

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
                printf("Sprite ID: %lu\n", currentSpriteID);
            } else if (kDown & KEY_DLEFT) {
                currentSpriteID = (currentSpriteID == 0) ? TOTAL_SPRITES - 1 : currentSpriteID - 1;
                LoadSpriteByID(currentSpriteID, 32, 32);
                printf("Sprite ID: %lu\n", currentSpriteID);
            }
        }

        // Render Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(0x20, 0x50, 0x20, 0xFF)); // Clear screen to Dark Green
        C2D_SceneBegin(topTarget);

        // Draw image at center of top screen
        if (ALL_SPRITES[currentSpriteID] != NULL) {
            C2D_DrawImageAt(g_gameImg, 184.0f, 104.0f, 0.5f, NULL, 1.0f, 1.0f);
        }

        C3D_FrameEnd(0);
    }

    C3D_TexDelete(&g_gameTex);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
