#ifndef WORLD_H
#define WORLD_H

#define MAP_WIDTH 32
#define MAP_HEIGHT 32

extern int worldGrid[MAP_WIDTH][MAP_HEIGHT]; // 'extern' lets main.cpp access this grid

void generateWorldWithSeed(unsigned int seed);
void drawWorldMap(float cameraX, float cameraY);

#endif
