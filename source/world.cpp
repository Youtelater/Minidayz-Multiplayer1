#include "world.h"
#include <cstdlib>

int worldGrid[MAP_WIDTH][MAP_HEIGHT];
static C2D_SpriteSheet localMapSheet = NULL;

void initWorldAssets(const char* sheetPath) {
    localMapSheet = C2D_SpriteSheetLoad(sheetPath);
}

void freeWorldAssets() {
    if (localMapSheet) {
        C2D_SpriteSheetFree(localMapSheet);
        localMapSheet = NULL;
    }
}

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
    if (!localMapSheet) return;

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int tileID = worldGrid[x][y];
            
            // Calculate screen coordinates (assuming 16x16 pixel tiles)
            float screenX = (x * 16.0f) - cameraX;
            float screenY = (y * 16.0f) - cameraY;

            // Only draw tiles visible on the 400x240 top screen
            if (screenX >= -16.0f && screenX < 400.0f && screenY >= -16.0f && screenY < 240.0f) {
                C2D_Image tileImg = C2D_SpriteSheetGetImage(localMapSheet, tileID);
                C2D_DrawImageAt(tileImg, screenX, screenY, 0.5f, NULL, 1.0f, 1.0f);
            }
        }
    }
}
