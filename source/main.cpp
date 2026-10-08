extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "gfx_table.h"
}

// Global Texture and Image containers
static C3D_Tex g_gameTex;
static C2D_Image g_gameImg;

enum GameState {
    STATE_LOADING,
    STATE_GAMEPLAY
};

// Safe upload function with bounds safety
void LoadSpriteByID(u32 spriteID) {
    if (spriteID >= TOTAL_SPRITES) return;
    
    // Safety check: ensure pointer is not NULL before sending to GPU
    if (ALL_SPRITES[spriteID] != NULL) {
        C3D_TexUpload(&g_gameTex, ALL_SPRITES[spriteID]);
    }
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // Initialize texture buffer to 512x512 to accommodate larger UI/loading screens safely
    C3D_TexInit(&g_gameTex, 512, 512, GPU_RGBA5551);
    C3D_TexSetFilter(&g_gameTex, GPU_NEAREST, GPU_NEAREST);

    // Default subtexture mapping for full screen / canvas dimensions
    static Tex3DS_SubTexture subtex = { 512, 512, 0.0f, 1.0f, 1.0f, 0.0f };
    g_gameImg.tex = &g_gameTex;
    g_gameImg.subtex = &subtex;

    GameState currentState = STATE_LOADING;
    u32 currentSpriteID = 0; 
    int loadingTimer = 0;

    // Safely load sprite index 0 on startup
    if (TOTAL_SPRITES > 0) {
        LoadSpriteByID(0);
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // State Machine Logic
        if (currentState == STATE_LOADING) {
            loadingTimer++;

            // Stay on loading screen for 180 ticks (~3 sec) or until 'A' button is pressed
            if (loadingTimer >= 180 || (kDown & KEY_A)) {
                currentState = STATE_GAMEPLAY;
                if (TOTAL_SPRITES > 1) {
                    currentSpriteID = 1;
                    LoadSpriteByID(currentSpriteID);
                }
            }
        } 
        else if (currentState == STATE_GAMEPLAY) {
            // D-Pad input cycles through all loaded sprite IDs
            if (kDown & KEY_DRIGHT) {
                currentSpriteID = (currentSpriteID + 1) % TOTAL_SPRITES;
                LoadSpriteByID(currentSpriteID);
            } else if (kDown & KEY_DLEFT) {
                currentSpriteID = (currentSpriteID == 0) ? TOTAL_SPRITES - 1 : currentSpriteID - 1;
                LoadSpriteByID(currentSpriteID);
            }
        }

        // Render Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_SceneBegin(topTarget);

        // Render current VRAM texture centered
        C2D_DrawImageAt(g_gameImg, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);

        C3D_FrameEnd(0);
    }

    C3D_TexDelete(&g_gameTex);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
