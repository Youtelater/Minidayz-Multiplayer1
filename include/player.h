#pragma once
#include <3ds.h>
#include <citro2d.h>

struct Player {
    float x;
    float y;
    float health;
};

void Player_Init();
void Player_Update(const circlePosition& circlePad);
void Player_Draw(C2D_SpriteSheet spriteSheet, C2D_Sprite& sprite);
