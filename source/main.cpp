#define MAP_WIDTH 225
#define MAP_HEIGHT 74

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <sstream>

#include "player.h"
#include "world.h"
#include "ItemTable.h"
#include "ShaderMenu.h"

// Game States
enum GameState {
    STATE_LOADING,
    STATE_MENU,
    STATE_SETTINGS,
    STATE_GAMEPLAY
};

GameState currentState = STATE_GAMEPLAY;

C2D_SpriteSheet menuSheet = NULL;
C2D_SpriteSheet mapSheet = NULL;
C3D_RenderTarget* topScreen = NULL;
C3D_RenderTarget* bottomScreen = NULL;

// Instantiate the Shader Menu
ShaderMenu shaderMenu;

// Function to load and decode your Construct 2 run-length encoded string data
void loadBunkerMapData() {
    std::string mapDataString = "2958x6,8,33x11,9,2x6,8,16x11,9,4x6,8,8x11,22,15x11,22,8x11,9,4x6,8,5x11,22,4x11,9,2x6,8,5x11,22,12x11,22,13x11,22,6x11,9,72x6,18,33x16,19,2x6,18,16x16,19,4x6,18,33x16,19,4x6,18,10x16,19,2x6,18,26x16,24,12x16,19,72x6,7,33x20,5,2x6,7,16x20,5,4x6,7,33x20,5,4x6,7,10x20,5,2x6,7,24x20,30,13x20,29,5,72x6,7,19x20,34,13x20,5,2x6,7,2x20,33,9x20,28,3x20,5,4x6,7,8x20,33,10x20,29,13x20,5,4x6,7,10x20,5,2x6,7,2x20,34,7x20,29,28x20,5,72x6,7,10x20,29,22x20,5,2x6,7,16x20,5,4x6,7,22x20,34,10x20,5,4x6,7,10x20,5,2x6,7,25x20,26,13x20,5,72x6,7,33x20,5,2x6,7,16x20,10,4x11,12,33x20,10,4x11,12,3x20,32,6x20,10,2x11,12,8x20,32,30x20,5,72x6,7,3x20,28,25x20,30,3x20,5,2x6,7,16x20,15,4x16,17,33x20,15,4x16,17,10x20,15,2x16,17,16x20,21,17x20,21,4x20,5,72x6,7,7x20,21,6x20,26,8x20,21,9x20,5,2x6,7,2x20,29,5x20,21,14x20,25,11x20,21,14x20,30,33x20,28,29x20,5,72x6,7,33x20,5,2x6,7,45x20,31,29x20,27,20x20,28,5x20,34,2x20,29,8x20,5,72x6,7,33x20,5,2x6,7,16x20,0,4x1,2,33x20,0,4x1,2,5x20,21,4x20,0,35x1,2,6x20,5,72x6,7,33x20,5,2x6,7,2x20,34,13x20,5,4x6,7,4x20,26,5x20,34,22x20,5,4x6,7,10x20,5,35x6,7,3x20,34,2x20,5,72x6,7,2x20,33,19x20,33,10x20,5,2x6,7,10x20,26,5x20,5,4x6,7,17x20,33,15x20,5,4x6,7,10x20,5,35x6,7,6x20,5,72x6,7,16x20,27,16x20,5,2x6,7,16x20,5,4x6,7,28x20,28,4x20,5,4x6,7,7x20,34,2x20,5,35x6,7,6x20,5,72x6,3,28x1,2,4x20,5,2x6,3,16x1,4,4x6,3,33x1,4,4x6,7,10x20,5,35x6,7,6x20,5,101x6,7,4x20,5,63x6,7,10x20,5,35x6,7,6x20,5,101x6,7,4x20,5,63x6,7,5x20,31,4x20,5,2x6,8,6x11,22,11x11,22,5x11,9,7x6,7,6x20,5,50x6,8,5x11,22,13x11,22,8x11,22,10x11,22,10x11,12,4x20,10,11x11,22,7x11,9,4x6,8,7x11,22,12x11,22,11x11,22,5x11,12,10x20,5,2x6,18,24x16,19,7x6,7,28,5x20,10,20x11,9,29x6,18,16,24,7x16,23,13x16,24,10x16,23,11x16,24,3x16,17,4x20,15,8x16,23,7x16,24,2x16,19,4x6,18,2x16,24,35x16,17,10x20,5,2x6,7,24x20,5,7x6,7,6x20,15,6x16,24,11x16,24,16,19,29x6,7,51x20,25,19x20,27,3x20,5,4x6,7,7x20,29,31x20,33,9x20,5,2x6,7,20,28,22x20,5,7x6,7,27x20,5,29x6,7,29x20,29,30x20,34,14x20,5,4x6,7,49x20,5,2x6,7,14x20,34,9x20,5,7x6,7,7x20,30,8x20,34,10x20,5,29x6,7,9x20,26,14x20,30,11x20,28,38x20,5,4x6,7,3x20,32,25x20,34,19x20,5,2x6,7,21x20,32,2x20,5,7x6,7,27x20,5,29x6,7,20,25,3x20,30,12x20,28,51x20,32,4x20,5,4x6,7,49x20,5,2x6,7,11x20,21,12x20,5,7x6,7,3x20,25,8x20,27,9x20,25,4x6,7,29x6,7,12x20,21,18x20,34,7x20,21,35x20,5,4x6,7,15x20,30,8x20,26,3x20,28,20x20,5,2x6,7,5x20,32,10x20,30,7x20,5,7x6,7,27x20,5,29x6,7,57x20,33,8x20,21,8x20,5,4x6,7,7x20,21,27x20,21,9x20,32,3x20,5,2x6,7,24x20,5,7x6,7,16x20,32,10x20,5,29x6,7,6x20,31,8x20,34,8x20,26,50x20,5,4x6,7,49x20,5,2x6,7,24x20,5,7x6,7,20,34,22x20,26,2x20,5,29x6,7,45x20,32,8x20,34,20x20,5,4x6,7,20,28,47x20,5,2x6,7,24x20,5,7x6,7,6x20,0,20x1,4,29x6,3,30x1,2,3x20,0,24x1,2,15x20,5,4x6,7,22x20,27,18x20,29,7x20,5,2x6,7,11x20,27,9x20,28,2x20,5,7x6,7,6x20,5,81x6,7,3x20,5,24x6,7,4x20,28,4x20,29,5x20,5,4x6,7,13x20,0,18x1,2,3x20,0,12x1,4,2x6,3,2x1,2,2x20,0,18x1,4,7x6,3,1,2,2x20,0,1,4,81x6,7,3x20,5,24x6,7,15x20,5,4x6,7,13x20,5,18x6,7,3x20,5,18x6,7,2x20,5,28x6,7,2x20,5,83x6,7,3x20,5,24x6,7,15x20,5,4x6,7,13x20,5,18x6,7,3x20,5,18x6,7,2x20,5,28x6,7,2x20,5,83x6,7,3x20,5,24x6,7,15x20,5,4x6,7,2x20,33,10x20,5,18x6,7,3x20,5,18x6,7,2x20,5,28x6,7,2x20,5,83x6,7,3x20,5,24x6,7,15x20,5,4x6,7,6x20,30,3x20,34,2x20,5,3x6,8x11,22,6x11,12,3x20,10,12x11,9,2x6,8,2x11,12,2x20,10,2x11,22,10x11,22,4x11,9,7x6,8,11,12,2x20,10,11,9,72x6,8,8x11,12,3x20,10,20x11,4x6,7,9x20,26,5x20,5,4x6,7,13x20,5,3x6,13x16,24,16,17,3x20,15,12x16,19,2x6,18,2x16,17,2x20,15,18x16,19,7x6,18,16,17,2x20,15,16,19,72x6,18,8x16,17,3x20,15,3x16,23,6x16,24,9x16,4x6,7,15x20,5,4x6,7,13x20,5,3x6,32x20,5,2x6,7,24x20,5,7x6,7,6x20,5,72x6,7,33x20,4x6,7,2x20,32,12x20,5,4x6,7,13x20,5,3x6,20,27,30x20,5,2x6,7,24x20,5,7x6,7,6x20,5,72x6,7,14x20,34,18x20,4x6,7,15x20,5,4x6,7,13x20,5,3x6,25x20,34,6x20,5,2x6,7,14x20,29,9x20,5,7x6,7,6x20,5,72x6,7,30x20,25,2x20,4x6,7,15x20,5,4x6,7,4x20,28,8x20,5,3x6,16x20,28,15x20,5,2x6,7,3x20,31,15x20,32,4x20,5,7x6,7,6x20,5,72x6,7,3x20,28,29x20,4x6,7,15x20,10,4x11,12,13x20,5,3x6,32x20,5,2x6,7,24x20,10,7x11,12,6x20,5,72x6,7,9x20,21,11x20,29,11x20,4x6,7,7x20,21,7x20,15,2x16,24,16,17,13x20,5,3x6,32x20,5,2x6,7,13x20,21,10x20,15,3x16,23,3x16,17,4x20,34,20,5,72x6,7,26x20,21,6x20,4x6,7,28x20,21,5x20,5,3x6,6x20,32,25x20,5,2x6,7,39x20,5,72x6,7,33x20,4x6,7,34x20,5,3x6,14x20,21,9x20,26,7x20,5,2x6,7,35x20,29,3x20,5,72x6,7,16x20,32,16x20,4x6,7,23x20,34,10x20,5,3x6,32x20,5,2x6,7,30x20,32,8x20,5,72x6,7,33x20,4x6,7,2x20,34,6x20,28,24x20,5,3x6,32x20,5,2x6,7,4x20,30,5x20,34,28x20,5,72x6,7,20,27,31x20,4x6,7,14x20,33,19x20,5,3x6,3x20,29,28x20,5,2x6,7,21x20,27,17x20,5,72x6,3,33x1,4x6,3,34x1,4,3x6,32x1,4,2x6,3,39x1,4,3639x6";

    std::stringstream ss(mapDataString);
    std::string segment;
    
    int currentX = 0;
    int currentY = 0;

    while (std::getline(ss, segment, ',')) {
        size_t found = segment.find('x');
        if (found != std::string::npos) {
            int count = std::stoi(segment.substr(0, found));
            int tileID = std::stoi(segment.substr(found + 1));

            for (int i = 0; i < count; i++) {
                if (currentX < MAP_WIDTH && currentY < MAP_HEIGHT) {
                    worldGrid[currentX][currentY] = tileID;
                }
                currentX++;
                if (currentX >= MAP_WIDTH) {
                    currentX = 0;
                    currentY++;
                }
            }
        } else {
            if (!segment.empty()) {
                int tileID = std::stoi(segment);
                if (currentX < MAP_WIDTH && currentY < MAP_HEIGHT) {
                    worldGrid[currentX][currentY] = tileID;
                }
                currentX++;
                if (currentX >= MAP_WIDTH) {
                    currentX = 0;
                    currentY++;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Load assets immediately for direct gameplay testing
    mapSheet = C2D_SpriteSheetLoad("romfs:/gfx/player.t3x"); 
    initWorldAssets("romfs:/gfx/player.t3x"); // <-- This tells world.cpp what to draw!
    loadBunkerMapData();

    float cameraX = 0.0f;
    float cameraY = 0.0f;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        // Camera movement via D-Pad
        if (kHeld & KEY_UP)    cameraY -= 3.0f;
        if (kHeld & KEY_DOWN)  cameraY += 3.0f;
        if (kHeld & KEY_LEFT)  cameraX -= 3.0f;
        if (kHeld & KEY_RIGHT) cameraX += 3.0f;

        // Render Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // --- TOP SCREEN RENDERING ---
        C2D_TargetClear(topScreen, C2D_Color32(30, 30, 30, 255));
        C2D_SceneBegin(topScreen);

        drawWorldMap(cameraX, cameraY);

        // --- BOTTOM SCREEN RENDERING ---
        C2D_TargetClear(bottomScreen, C2D_Color32(20, 20, 20, 255));
        C2D_SceneBegin(bottomScreen);

        C3D_FrameEnd(0); 
    }

    if (mapSheet) C2D_SpriteSheetFree(mapSheet);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}
