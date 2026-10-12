#ifndef WORLD_H
#define WORLD_H

#include <citro2d.h>

#define MAP_WIDTH 32
#define MAP_HEIGHT 32

// Global world grid data shared across files
extern int worldGrid[MAP_WIDTH][MAP_HEIGHT];

// Function declarations
void initWorldAssets(const char* sheetPath);
void freeWorldAssets();
void generateWorldWithSeed(unsigned int seed);
void drawWorldMap(float cameraX, float cameraY);

#endif
