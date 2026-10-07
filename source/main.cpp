extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "gfx_aliases.h"
}

static C2D_Image ImageFromGrit(const void* tiles, u32 width, u32 height) {
    C3D_Tex* tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
    
    // GPU_RGBA5551 uses 16 bits (2 bytes) per pixel with 1-bit alpha transparency
    C3D_TexInit(tex, (u16)width, (u16)height, GPU_RGBA5551);
    C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);
    
    C3D_TexUpload(tex, tiles);
    
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

    C2D_Image playerImg = ImageFromGrit(покой1Tiles, 32, 32);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(topTarget, C2D_Color32(0x20, 0x20, 0x20, 0xFF));
        C2D_SceneBegin(topTarget);
        
        C2D_DrawImageAt(playerImg, 184.0f, 104.0f, 0.5f, NULL, 1.0f, 1.0f);

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
