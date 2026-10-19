#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>

#include "world.h"
#include "player.h"

// Define Game States
enum GameState {
    STATE_MENU,
    STATE_GAMEPLAY
};

GameState currentState = STATE_MENU;

C3D_RenderTarget* topScreen = NULL;
C2D_SpriteSheet menuSheet = NULL;

// Function to load and parse extracted JSON assets from romfs at startup
void loadExtractedAssets() {
    std::printf("Loading extracted JSON assets from romfs...\n");

    // 1. Read Weapon Data
    std::ifstream weaponFile("romfs:/extracted_weapons.json");
    if (!weaponFile.is_open()) {
        std::printf("Warning: extracted_weapons.json not found in romfs!\n");
    } else {
        std::printf("Success: Loaded extracted_weapons.json\n");
        weaponFile.close();
    }

    // 2. Parse Split Map Data Parts into globalLoadedTiles
    std::printf("Parsing map part 1...\n");
    parseMapFile("romfs:/extracted_maps_part1.json");

    std::printf("Parsing map part 2...\n");
    parseMapFile("romfs:/extracted_maps_part2.json");
}

int main(int argc, char* argv[]) {
    // Initialize system hardware and romfs FIRST
    romfsInit();
    gfxInitDefault();
    
    // Bottom screen text console for debug logs
    consoleInit(GFX_BOTTOM, NULL);
    std::printf("\n=== Mini DayZ+ 3DS Booting ===\n\n");

    // Load and parse extracted JSON assets right after initialization
    loadExtractedAssets();

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create rendering target for the top screen
    topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // Load Menu Asset Sheet
    std::printf("Loading menu assets...\n");
    menuSheet = C2D_SpriteSheetLoad("gfx/menu.t3x");
    if (!menuSheet) {
        std::printf("Warning: menu.t3x not found or failed to load.\n");
    } else {
        std::printf("Success: menu.t3x loaded.\n");
    }

    // Initialize World Assets (loads frame sheets)
    std::printf("Initializing world and frame assets...\n");
    initWorldAssets("gfx/frame_001.t3x");

    // Generate world map data using world.cpp functions
    std::printf("Generating world map...\n");
    generateWorldWithSeed(12345); 
    std::printf("World ready!\nPress A on the menu to start.\n");

    float cameraX = 0.0f;
    float cameraY = 0.0f;

    // Main Game Loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        // Exit game if START is pressed
        if (kDown & KEY_START) break;

        // --- STATE LOGIC ---
        if (currentState == STATE_MENU) {
            // Press A to transition from Menu to Gameplay
            if (kDown & KEY_A) {
                currentState = STATE_GAMEPLAY;
                std::printf("Switched to STATE_GAMEPLAY\n");
            }
        } 
        else if (currentState == STATE_GAMEPLAY) {
            // Camera movement via D-Pad during gameplay
            if (kHeld & KEY_UP)    cameraY -= 3.0f;
            if (kHeld & KEY_DOWN)  cameraY += 3.0f;
            if (kHeld & KEY_LEFT)  cameraX -= 3.0f;
            if (kHeld & KEY_RIGHT) cameraX += 3.0f;
        }

        // --- RENDERING ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Top Screen Rendering
        C2D_SceneBegin(topScreen);
        C2D_TargetClear(topScreen, C2D_Color32(20, 20, 20, 255));

        if (currentState == STATE_MENU) {
            // Draw menu graphic if loaded
            if (menuSheet != NULL) {
                C2D_Image menuImg = C2D_SpriteSheetGetImage(menuSheet, 0);
                C2D_DrawImageAt(menuImg, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
            }
        } 
        else if (currentState == STATE_GAMEPLAY) {
            // Draw the multi-frame game world map using world.cpp functions
            drawWorldMap(cameraX, cameraY);
        }

        C3D_FrameEnd(0); 
    }

    // Cleanup resources on exit
    if (menuSheet) C2D_SpriteSheetFree(menuSheet);
    freeWorldAssets();

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}