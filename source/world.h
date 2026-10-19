#ifndef WORLD_H
#define WORLD_H

#include <3ds.h>
#include <citro2d.h>
#include <vector>
#include <string>

#define MAP_WIDTH 225
#define MAP_HEIGHT 74

// Define a structure to hold parsed JSON map tile/object data
struct MapTile {
    float x;
    float y;
    int tileType;
};

// Expose the loaded tile list and map parsing function globally
extern std::vector<MapTile> globalLoadedTiles;
void parseMapFile(const std::string& filepath);

extern int worldGrid[MAP_WIDTH][MAP_HEIGHT];

void initWorldAssets(const char* mapSheetPath);
void loadBuildingSheet(int index, const char* sheetPath);
void freeWorldAssets();
void generateWorldWithSeed(unsigned int seed);
void drawWorldMap(float cameraX, float cameraY);

#endif