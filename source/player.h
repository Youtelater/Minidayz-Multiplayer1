#ifndef PLAYER_H
#define PLAYER_H

#include <3ds.h>
#include <citro2d.h>

struct Player {
    float x, y;     // World coordinates
    float speed;    // Movement speed
};

extern Player player;

void initPlayer(float startX, float startY);
void updatePlayer(u32 kHeld);
void drawPlayer(float cameraX, float cameraY, C2D_SpriteSheet playerSheet);

#endif
