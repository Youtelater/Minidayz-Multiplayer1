#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include "world.h"
#include <cstdlib>
#include <cstdio>

int worldGrid[MAP_WIDTH][MAP_HEIGHT];
static C2D_SpriteSheet terrainSheet = NULL;

#define MAX_ATLASES 14
static C2D_SpriteSheet worldAtlases[MAX_ATLASES];

void initWorldAssets(const char* mapSheetPath) {
    terrainSheet = C2D_SpriteSheetLoad(mapSheetPath);

    for (int i = 0; i < MAX_ATLASES; i++) {
        char path[64];
        std::sprintf(path, "romfs:/gfx/atlas_%02d.t3x", i + 1);
        
        worldAtlases[i] = C2D_SpriteSheetLoad(path);
        if (!worldAtlases[i]) {
            std::printf("Notice: Could not load %s\n", path);
        } else {
            std::printf("Successfully loaded: %s\n", path);
        }
    }
}

void loadBuildingSheet(int index, const char* sheetPath) {
    if (index >= 0 && index < MAX_ATLASES) {
        worldAtlases[index] = C2D_SpriteSheetLoad(sheetPath);
    }
}

void freeWorldAssets() {
    if (terrainSheet) C2D_SpriteSheetFree(terrainSheet);
    
    for (int i = 0; i < MAX_ATLASES; i++) {
        if (worldAtlases[i]) {
            C2D_SpriteSheetFree(worldAtlases[i]);
            worldAtlases[i] = NULL;
        }
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
                worldGrid[x][y] = 100; // Special structure identifier
            }
        }
    }
}

void drawWorldMap(float cameraX, float cameraY) {
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int tileID = worldGrid[x][y];
            float screenX = (x * 16.0f) - cameraX;
            float screenY = (y * 16.0f) - cameraY;

            if (screenX >= -16.0f && screenX < 400.0f && screenY >= -16.0f && screenY < 240.0f) {
                if (tileID < 100 && terrainSheet) {
                    C2D_Image tileImg = C2D_SpriteSheetGetImage(terrainSheet, tileID);
                    C2D_DrawImageAt(tileImg, screenX, screenY, 0.5f, NULL, 1.0f, 1.0f);
                } 
                else if (tileID >= 100) {
                    int atlasIndex = (tileID / 100) - 1; 
                    if (atlasIndex < 0) atlasIndex = 0;
                    if (atlasIndex >= MAX_ATLASES) atlasIndex = MAX_ATLASES - 1;

                    if (worldAtlases[atlasIndex]) {
                        int localID = tileID % 100;
                        C2D_Image bldgImg = C2D_SpriteSheetGetImage(worldAtlases[atlasIndex], localID);
                        C2D_DrawImageAt(bldgImg, screenX, screenY, 0.4f, NULL, 1.0f, 1.0f);
                    }
                }
            }
        }
    }
}
