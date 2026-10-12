#include "world.h"
#include <citro2d.h>
#include <cstdlib>

int worldGrid[MAP_WIDTH][MAP_HEIGHT];

void generateWorldWithSeed(unsigned int seed) {
    srand(seed);
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int roll = rand() % 100;
            if (roll < 60) worldGrid[x][y] = 0;
            else if (roll < 80) worldGrid[x][y] = 1;
            else worldGrid[x][y] = 2;
            else worldGrid[x][y] = 3;
        }
    }
}

void drawWorldMap(float cameraX, float cameraY, C2D_SpriteSheet mapSheet) {
    if (!mapSheet) return;
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            int tileID = worldGrid[x][y];
            float screenX = (x * 16.0f) - cameraX;
            float screenY = (y * 16.0f) - cameraY;

            if (screenX >= -16.0f && screenX < 400.0f && screenY >= -16.0f && screenY < 240.0f) {
                C2D_Image tileImg = C2D_SpriteSheetGetImage(mapSheet, tileID);
                C2D_DrawImageAt(tileImg, screenX, screenY, 0.5f, NULL, 1.0f, 1.0f);
            }
        }
    }
}
