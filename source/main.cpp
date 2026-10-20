#include <citro2d.h>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdio>

// Global vector to store our loaded JSON strings
std::vector<std::string> g_gameStrings;

// Function to load and cache the decoded strings from RomFS
bool loadAndCacheStrings() {
    // RomFS files are accessed using the "romfs:/" prefix
    std::ifstream file("romfs:/data/decoded_strings.json");
    if (!file.is_open()) {
        std::printf("Failed to open decoded_strings.json from RomFS!\n");
        return false;
    }

    nlohmann::json j;
    file >> j;

    if (j.is_discarded() || !j.is_array()) {
        std::printf("Failed to parse JSON!\n");
        return false;
    }

    g_gameStrings.clear();
    for (const auto& item : j) {
        if (item.is_string()) {
            g_gameStrings.push_back(item.get<std::string>());
        }
    }

    std::printf("Successfully loaded %zu strings into 3DS memory!\n", g_gameStrings.size());
    return true;
}

int main(int argc, char* argv[]) {
    // 1. Initialize 3DS hardware and RomFS
    gfxInitDefault();
    romfsInit(); // Crucial: Enables reading files from romfs:/
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets for both screens (or just the bottom one)
C3D_RenderTarget* topScreen = C2D_CreateTarget(GFX_TOP, GFX_LEFT);
C3D_RenderTarget* bottomScreen = C2D_CreateTarget(GFX_BOTTOM, GFX_LEFT);
C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
// Draw on bottom screen
C2D_TargetClear(bottomScreen, C2D_Color32(40, 40, 40, 255));
C2D_SceneBegin(bottomScreen);

// TODO: Draw your menu buttons and text here!

C3D_FrameEnd(0);


    // 2. Load our extracted JSON strings into memory
    loadAndCacheStrings();

    // Main game loop
    while (aptMainLoop()) {
        // Scan gamepad inputs
        hidScanInput();
        u32 kDown = hidKeysDown();

        // Press START to exit the homebrew application
        if (kDown & KEY_START) break;

        // Render frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, C2D_Color32(30, 30, 30, 255)); // Dark grey background
        C2D_SceneBegin(topScreen);

        // TODO: Render your menu or gameplay using Citro2D here!

        C3D_FrameEnd(0);
    }

    // 3. Clean up resources on exit
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
