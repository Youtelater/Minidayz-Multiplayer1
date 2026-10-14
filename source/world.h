#ifndef WORLD_H
#define WORLD_H

#include <3ds.h>
#include <citro2d.h>

#define MAP_WIDTH 225
#define MAP_HEIGHT 74
#define TOTAL_ATLASES 14

extern int worldGrid[MAP_WIDTH][MAP_HEIGHT];
extern C2D_SpriteSheet worldSheets[TOTAL_ATLASES];

void initWorldAssets(const char* basePath);
void drawWorldMap(float cameraX, float cameraY);

#endif
