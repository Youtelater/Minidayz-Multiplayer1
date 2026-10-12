#include "world.h"
#include <cstdlib>

int worldGrid[MAP_WIDTH][MAP_HEIGHT];
static C2D_SpriteSheet terrainSheet = NULL;
static C2D_SpriteSheet buildingSheets[10]; // Array to hold different building sheets

void initWorldAssets(const char* mapSheetPath) {
    terrainSheet = C2D_SpriteSheetLoad(mapSheetPath);
}

void loadBuildingSheet(int index, const char* sheetPath) {
    buildingSheets[index] = C2D_SpriteSheetLoad(sheetPath);
}

void freeWorldAssets() {
    if (terrainSheet) C2D_SpriteSheetFree(terrainSheet);
    for (int i = 0; i < 10; i++) {
        if (buildingSheets[i]) C2D_SpriteSheetFree(buildingSheets[i]);
    }
}

void generateWorldWithSeed(unsigned int seed) {
    srand(seed);

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int roll = rand() % 100;

            if (roll < 50) {
                worldGrid[x][y] = 0; // Grass
            } else if (roll < 75) {
                worldGrid[x][y] = 1; // Dirt path
            } else {
                worldGrid[x][y] = 100; // Special structure identifier (e.g., a building)
            }
        }
    }
}

void drawWorldMap(float cameraX, float cameraY) {
    if (!terrainSheet) return;

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int tileID = worldGrid[x][y];
            float screenX = (x * 16.0f) - cameraX;
            float screenY = (y * 16.0f) - cameraY;

            if (screenX >= -16.0f && screenX < 400.0f && screenY >= -16.0f && screenY < 240.0f) {
                // If it's standard terrain, draw from terrainSheet
                if (tileID < 100) {
                    C2D_Image tileImg = C2D_SpriteSheetGetImage(terrainSheet, tileID);
                    C2D_DrawImageAt(tileImg, screenX, screenY, 0.5f, NULL, 1.0f, 1.0f);
                } 
                // If it's a building, you can draw from your loaded building sheets!
                else if (buildingSheets[0]) {
                    C2D_Image bldgImg = C2D_SpriteSheetGetImage(buildingSheets[0], 0);
                    C2D_DrawImageAt(bldgImg, screenX, screenY, 0.4f, NULL, 1.0f, 1.0f);
                }
            }
        }
    }
}
