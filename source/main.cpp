#include <3ds.h>
#include <cstdio>

// Mini DAYZ+ Nintendo 3DS port
//
// This is the clean platform entry point. Original gameplay systems will be
// ported into separate, tested modules from the original Mini DAYZ+ data.
// Do not substitute placeholder mechanics for the original game's rules.

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    gfxInitDefault();

    PrintConsole topConsole;
    consoleInit(GFX_TOP, &topConsole);

    consoleClear();
    printf("MINI DAYZ+\n");
    printf("Nintendo 3DS port\n\n");
    printf("Starting platform layer...\n");

    if (romfsInit() == 0) {
        printf("RomFS: ready\n");
    } else {
        printf("RomFS: unavailable\n");
    }

    printf("\nClean C++ foundation\n");
    printf("Original game systems: next phase\n\n");
    printf("Press START to exit.\n");

    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown();

        if (down & KEY_START) {
            break;
        }

        gspWaitForVBlank();
    }

    romfsExit();
    gfxExit();
    return 0;
}
