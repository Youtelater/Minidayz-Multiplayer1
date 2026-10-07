extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "gfx_aliases.h"
}

// Helper to convert Grit 8-bit indexed + Palette into a standard 32-bit RGBA buffer for Citro2D
static C2D_Image ImageFromGritIndexed(const u8* tiles, const u16* palette, u32 width, u32 height) {
    C3D_Tex* tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
    
    // Allocate 32-bit texture
    C3D_TexInit(tex, (u16)width, (u16)height, GPU_RGBA8);
    C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);

    // Convert 8-bit tile indices to 32-bit RGBA buffer
    u32 totalPixels = width * height;
    u32* rgbaBuffer = (u32*)linearAlloc(totalPixels * sizeof(u32));

    for (u32 i = 0; i < totalPixels; i++) {
        u8 index = tiles[i];
        if (index == 0) {
            rgbaBuffer[i] = 0x00000000; // Transparent background
        } else {
            u16 bgr555 = palette[index];
            // Convert BGR555/RGB555 to RGBA8888
            u8 r = (bgr555 & 0x1F) << 3;
            u8 g = ((bgr555 >> 5) & 0x1F) << 3;
            u8 b = ((bgr555 >> 10) & 0x1F) << 3;
            rgbaBuffer[i] = (255 << 24) | (b << 16) | (g << 8) | r;
        }
    }

    C3D_TexUpload(tex, rgbaBuffer);
    linearFree(rgbaBuffer);

    static Tex3DS_SubTexture subtex;
    subtex.width = (u16)width;
    subtex.height = (u16)height;
    subtex.left = 0.0f;
    subtex.top = 0.0f;
    subtex.right = 1.0f;
    subtex.bottom = 1.0f;

    C2D_Image img;
    img.tex = tex;
    img.subtex = &subtex;
    return img;
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Load indexed sprite using its tiles and palette array
    C2D_Image playerImg = ImageFromGritIndexed(покой1Tiles, покой1Pal, 32, 32);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(topTarget, C2D_Color32(0x20, 0x20, 0x20, 0xFF));
        C2D_SceneBegin(topTarget);
        
        C2D_DrawImageAt(playerImg, 184.0f, 104.0f, 0.5f, NULL, 1.0f, 1.0f);

        C2D_TargetClear(bottomTarget, C2D_Color32(0x10, 0x10, 0x10, 0xFF));
        C2D_SceneBegin(bottomTarget);

        C3D_FrameEnd(0);
    }

    if (playerImg.tex) {
        C3D_TexDelete(playerImg.tex);
        free(playerImg.tex);
    }
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
