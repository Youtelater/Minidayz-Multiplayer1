#include <3ds.h>
#include <citro2d.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>

// Screen logger: prints directly to the bottom screen console and flushes instantly
void logCrash(const char* message) {
    printf("%s\n", message);
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    // 1. Core 3DS & Console Init
    gfxInitDefault();

    PrintConsole bottomConsole;
    consoleInit(GFX_BOTTOM, &bottomConsole); 
    
    logCrash("=== Direct Test Session ===");
    
    if (romfsInit() != 0) {
        logCrash("ERROR: romfsInit() failed!");
    } else {
        logCrash("romfsInit() succeeded.");
    }

    // 2. Direct inline test without calling an external function
    logCrash("Testing fopen on romfs:/game.bin...");
    FILE* f = fopen("romfs:/game.bin", "rb");
    if (!f) {
        logCrash("CRITICAL: fopen returned NULL! File not found in RomFS.");
    } else {
        logCrash("SUCCESS: fopen opened game.bin!");
        
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        
        char buf[64];
        std::snprintf(buf, sizeof(buf), "File size: %ld bytes", size);
        logCrash(buf);
    }

    logCrash("Test sequence finished. Press START to exit.");

    // 3. Simple idle loop to keep the console visible
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;
        
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_FrameEnd(0);
    }

    romfsExit();
    gfxExit();
    return 0;
}
