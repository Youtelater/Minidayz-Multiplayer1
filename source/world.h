#ifndef WORLD_H
#define WORLD_H

#include <citro2d.h>

#define MAP_WIDTH 225
#define MAP_HEIGHT 74

// Expose the grid globally so other files can read/write to it
extern int worldGrid[MAP_WIDTH][MAP_HEIGHT];

// Function declarations
void initWorldAssets(const char* mapSheetPath);
void loadBuildingSheet(int index, const char* sheetPath);
void freeWorldAssets();
void generateWorldWithSeed(unsigned int seed);
void drawWorldMap(float cameraX, float cameraY);

#endif
