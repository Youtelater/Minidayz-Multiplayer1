#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include "gfx_table.h" // Your auto-generated header

int main(int argc, char* argv[]) {
    // 1. Initialize core 3DS services
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // 2. Initialize RomFS so it can read files from 'romfs:/'
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        // If this fails, handle it or exit safely
    }

    // 3. Now you can safely load your textures
    // C2D_SpriteSheet sheet = C2D_SpriteSheetLoad("romfs:/gfx/loading-logo.t3x");

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break; // Press Start to exit

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(C2D_GetScreenTop(), C2D_Color32(30, 30, 30, 255));
        
        // Draw things here...

        C2D_Flush();
        C3D_FrameEnd(0);
    }

    // Clean up services before exiting
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
