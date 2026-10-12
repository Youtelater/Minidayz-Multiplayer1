#include "world.h"
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <cstdlib>
#include <ctime>
#include <stdio.h>

#define MAP_WIDTH 32
#define MAP_HEIGHT 32

// Game States
enum GameState {
    STATE_LOADING,
    STATE_MENU,
    STATE_GAMEPLAY
};

GameState currentState = STATE_LOADING;

// World Grid and Assets
int worldGrid[MAP_WIDTH][MAP_HEIGHT];
C2D_SpriteSheet menuSheet = NULL;
C2D_SpriteSheet mapSheet = NULL;
C3D_RenderTarget* topScreen = NULL;

// Initialize Procedural Generation using a Seed
void generateWorldWithSeed(unsigned int seed) {
    srand(seed);

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int roll = rand() % 100; // 0 to 99 roll

            if (roll < 60) {
                worldGrid[x][y] = 0; // Grass tile
            } else if (roll < 80) {
                worldGrid[x][y] = 1; // Dirt / Path tile
            } else if (roll < 95) {
                worldGrid[x][y] = 2; // Building / Wall tile
            } else {
                worldGrid[x][y] = 3; // Door / Special tile
            }
        }
    }
}

void drawWorldMap(float cameraX, float cameraY) {
    if (!mapSheet) return;

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int tileID = worldGrid[x][y];
            
            // Calculate screen coordinates (assuming 16x16 pixel tiles)
            float screenX = (x * 16.0f) - cameraX;
            float screenY = (y * 16.0f) - cameraY;

            // Only draw visible tiles on the 3DS top screen (400x240)
            if (screenX >= -16.0f && screenX < 400.0f && screenY >= -16.0f && screenY < 240.0f) {
                C2D_Image tileImg = C2D_SpriteSheetGetImage(mapSheet, tileID);
                C2D_DrawImageAt(tileImg, screenX, screenY, 0.5f, NULL, 1.0f, 1.0f);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize graphics and services
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // Load initial menu assets
    currentState = STATE_MENU;
    menuSheet = C2D_SpriteSheetLoad("romfs:/gfx/main_menu_logo_sheet0.t3x"); 

    float cameraX = 0.0f;
    float cameraY = 0.0f;

    // Main game loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break; // Exit game

        // State Machine Handling
        if (currentState == STATE_MENU) {
            if (kDown & KEY_A) {
                // Free menu assets to save 3DS RAM
                if (menuSheet) {
                    C2D_SpriteSheetFree(menuSheet);
                    menuSheet = NULL;
                }

                // Load map tiles sheet from RomFS
                mapSheet = C2D_SpriteSheetLoad("romfs:/gfx/player.t3x"); // Update this to your map sheet asset name later

                // Generate world using system time or a fixed seed
                unsigned int randomSeed = (unsigned int)time(NULL);
                generateWorldWithSeed(randomSeed);

                currentState = STATE_GAMEPLAY;
            }
        } 
        else if (currentState == STATE_GAMEPLAY) {
            // Basic camera movement via D-Pad
            if (kHeld & KEY_UP)    cameraY -= 2.0f;
            if (kHeld & KEY_DOWN)  cameraY += 2.0f;
            if (kHeld & KEY_LEFT)  cameraX -= 2.0f;
            if (kHeld & KEY_RIGHT) cameraX += 2.0f;
        }

        // Render Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, C2D_Color32(30, 30, 30, 255));
        C2D_SceneBegin(topScreen);

        if (currentState == STATE_MENU) {
            // Draw menu graphics here if loaded
        } 
        else if (currentState == STATE_GAMEPLAY) {
            drawWorldMap(cameraX, cameraY);
        }

        C3D_FrameEnd(0); // End frame context properly with flag 0
    }

    // Clean up resources on exit
    if (menuSheet) C2D_SpriteSheetFree(menuSheet);
    if (mapSheet) C2D_SpriteSheetFree(mapSheet);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}
