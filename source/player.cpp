#include "player.h"

Player player;

void initPlayer(float startX, float startY) {
    player.x = startX;
    player.y = startY;
    player.speed = 2.0f;
}

void updatePlayer(u32 kHeld) {
    // Basic directional movement
    if (kHeld & KEY_UP)    player.y -= player.speed;
    if (kHeld & KEY_DOWN)  player.y += player.speed;
    if (kHeld & KEY_LEFT)  player.x -= player.speed;
    if (kHeld & KEY_RIGHT) player.x += player.speed;
}

void drawPlayer(float cameraX, float cameraY, C2D_SpriteSheet playerSheet) {
    if (!playerSheet) return;

    // Convert world coordinates to screen coordinates based on camera position
    float screenX = player.x - cameraX;
    float screenY = player.y - cameraY;

    // Draw the player sprite (using index 0 of the player sheet)
    C2D_Image playerImg = C2D_SpriteSheetGetImage(playerSheet, 0);
    C2D_DrawImageAt(playerImg, screenX, screenY, 0.6f, NULL, 1.0f, 1.0f);
}
