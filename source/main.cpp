extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "gfx_table.h"
}

// Global Texture and Image container
static C3D_Tex g_playerTex;
static C2D_Image g_playerImg;

// Helper to swap active tile data into VRAM
void LoadSpriteByID(u32 spriteID) {
    if (spriteID >= TOTAL_SPRITES) return;

    // Upload the selected sprite's tiled memory from our master array
    C3D_TexUpload(&g_playerTex, ALL_SPRITES[spriteID]);
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // Initialize one 32x32 16-bit texture buffer in VRAM once
    C3D_TexInit(&g_playerTex, 32, 32, GPU_RGBA5551);
    C3D_TexSetFilter(&g_playerTex, GPU_NEAREST, GPU_NEAREST);

    static Tex3DS_SubTexture subtex = { 32, 32, 0.0f, 1.0f, 1.0f, 0.0f };
    g_playerImg.tex = &g_playerTex;
    g_playerImg.subtex = &subtex;

    // --- LOAD FIRST IMAGE ON BOOT ---
    u32 currentSpriteID = 0;
    LoadSpriteByID(currentSpriteID);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // D-Pad Left/Right controls which sprite ID to render
        if (kDown & KEY_DRIGHT) {
            currentSpriteID = (currentSpriteID + 1) % TOTAL_SPRITES;
            LoadSpriteByID(currentSpriteID);
        } else if (kDown & KEY_DLEFT) {
            currentSpriteID = (currentSpriteID == 0) ? TOTAL_SPRITES - 1 : currentSpriteID - 1;
            LoadSpriteByID(currentSpriteID);
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(0x20, 0x20, 0x20, 0xFF));
        C2D_SceneBegin(topTarget);

        // Render whichever sprite ID is currently active
        C2D_DrawImageAt(g_playerImg, 184.0f, 104.0f, 0.5f, NULL, 1.0f, 1.0f);

        C3D_FrameEnd(0);
    }

    C3D_TexDelete(&g_playerTex);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
