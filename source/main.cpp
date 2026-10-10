#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    // Initialize services
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets for top and bottom screens
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Main loop
    while (aptMainLoop()) {
        // Scan user input
        hidScanInput();
        u32 kDown = hidKeysDown();

        // Exit on START button press
        if (kDown & KEY_START) {
            break;
        }

        // --- Render Frame ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Draw to Top Screen
        C2D_TargetClear(topScreen, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_SceneBegin(topScreen);
        
        // Example: Draw a red rectangle on top screen
        C2D_DrawRectSolid(10.0f, 10.0f, 0.5f, 100.0f, 100.0f, C2D_Color32(0xFF, 0x00, 0x00, 0xFF));

        // Draw to Bottom Screen
        C2D_TargetClear(bottomScreen, C2D_Color32(0x20, 0x20, 0x20, 0xFF));
        C2D_SceneBegin(bottomScreen);

        // End the frame
        C3D_FrameEnd(0);
    }

    // Deinitialize graphics libraries
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
