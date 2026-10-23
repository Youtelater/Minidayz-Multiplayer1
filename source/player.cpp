#include "player.h"

static Player g_player;

void Player_Init() {
    g_player.x = 160.0f;
    g_player.y = 120.0f;
    g_player.health = 100.0f;
}

void Player_Update(const circlePosition& circlePad) {
    g_player.x += (circlePad.dx / 30.0f);
    g_player.y -= (circlePad.dy / 30.0f); // Inverted Y for screen coordinates
}

void Player_Draw(C2D_SpriteSheet spriteSheet, C2D_Sprite& sprite) {
    if (spriteSheet) {
        sprite.params.pos.x = g_player.x;
        sprite.params.pos.y = g_player.y;
        C2D_DrawSprite(&sprite);
    }
}
