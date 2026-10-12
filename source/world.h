#ifndef WORLD_H
#define WORLD_H

#include <citro2d.h>

#define MAP_WIDTH 32
#define MAP_HEIGHT 32

extern int worldGrid[MAP_WIDTH][MAP_HEIGHT];

// Asset management for terrain and specific buildings
void initWorldAssets(const char* mapSheetPath);
void loadBuildingSheet(int buildingID, const char* sheetPath);
void freeWorldAssets();
void generateWorldWithSeed(unsigned int seed);
void drawWorldMap(float cameraX, float cameraY);

#endif
