#ifndef WORLD_H
#define WORLD_H

#include <3ds.h>
#include <citro2d.h>

#define MAP_WIDTH 225
#define MAP_HEIGHT 74

extern int worldGrid[MAP_WIDTH][MAP_HEIGHT];

// Asset management for terrain and specific buildings
void initWorldAssets(const char* mapSheetPath);
void loadBuildingSheet(int buildingID, const char* sheetPath);
void freeWorldAssets();
void generateWorldWithSeed(unsigned int seed);
void drawWorldMap(float cameraX, float cameraY);

#endif
