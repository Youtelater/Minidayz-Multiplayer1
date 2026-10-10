#include <3ds.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Initialize core services
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    iprintf("Mini DAYZ 3DS: Boot test successful!\n");
    iprintf("Press START to exit.\n");

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // Swap frames
        gfxFlushBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}