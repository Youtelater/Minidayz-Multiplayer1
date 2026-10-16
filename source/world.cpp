#include "world.h"
#include <cstdlib>
#include <cstdio>

// Separate grids for Ground (Floor) and Overhead Objects (Buildings)
int groundGrid[MAP_WIDTH][MAP_HEIGHT];
int buildingGrid[MAP_WIDTH][MAP_HEIGHT];

#define MAX_ATLASES 290
static C2D_SpriteSheet worldAtlases[MAX_ATLASES];

void initWorldAssets(const char* mapSheetPath) {
    // Automatically load all numbered frame assets from romfs:/gfx/
    for (int i = 0; i < MAX_ATLASES; i++) {
        char path[64];
        std::sprintf(path, "romfs:/gfx/frame_%03d.t3x", i + 1);
        
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
            // LAYER 1: Ground/Floor (Assign tile indexes that correspond to ground_lvl tilemaps)
            int groundRoll = rand() % 100;
            if (groundRoll < 70) {
                groundGrid[x][y] = 63; // Index for ground_lvl1_tilemap
            } else if (groundRoll < 90) {
                groundGrid[x][y] = 111; // Index for ground_lvl2_tilemap
            } else {
                groundGrid[x][y] = 85; // Index for tilemap_sand
            }

            // LAYER 2: Buildings & Objects (Placed on top of ground)
            int objectRoll = rand() % 100;
            if (objectRoll > 85) {
                // Assign an index corresponding to a building/structure (e.g., b_police_station, b_hospital)
                buildingGrid[x][y] = (rand() % 200) + 1; 
            } else {
                buildingGrid[x][y] = -1; // Empty / No building on this tile
            }
        }
    }
}

void drawWorldMap(float cameraX, float cameraY) {
    // PASS 1: Render Ground / Floor Layer FIRST across the whole view
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            float screenX = (x * 16.0f) - cameraX;
            float screenY = (y * 16.0f) - cameraY;

            if (screenX >= -32.0f && screenX < 400.0f && screenY >= -32.0f && screenY < 240.0f) {
                int groundID = groundGrid[x][y];
                if (groundID >= 0 && groundID < MAX_ATLASES && worldAtlases[groundID]) {
                    C2D_Image groundImg = C2D_SpriteSheetGetImage(worldAtlases[groundID], 0);
                    // Draw ground at depth z = 0.1f
                    C2D_DrawImageAt(groundImg, screenX, screenY, 0.1f, NULL, 1.0f, 1.0f);
                }
            }
        }
    }

    // PASS 2: Render Buildings & Structures SECOND on top of the ground
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            float screenX = (x * 16.0f) - cameraX;
            float screenY = (y * 16.0f) - cameraY;

            if (screenX >= -128.0f && screenX < 400.0f && screenY >= -128.0f && screenY < 240.0f) {
                int bldgID = buildingGrid[x][y];
                if (bldgID >= 0 && bldgID < MAX_ATLASES && worldAtlases[bldgID]) {
                    C2D_Image bldgImg = C2D_SpriteSheetGetImage(worldAtlases[bldgID], 0);
                    // Draw building at depth z = 0.5f (above ground)
                    C2D_DrawImageAt(bldgImg, screenX, screenY, 0.5f, NULL, 1.0f, 1.0f);
                }
            }
        }
    }
}
