extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
}

// --- PLAYER ANIMATION ENUMS & STRUCTURES ---
enum Direction {
    DIR_DOWN = 0,
    DIR_UP = 1,
    DIR_LEFT = 2,
    DIR_RIGHT = 3
};

struct Player {
    float x = 192.0f;        // Centered on Top Screen (400x240)
    float y = 112.0f;
    float speed = 1.5f;
    Direction dir = DIR_DOWN;
    int currentFrame = 0;    // Columns 0-3
    int animTimer = 0;       // Animation speed controller
    bool isMoving = false;
};

// --- MENU OPTIONS ---
enum MenuOption {
    MENU_START = 0,
    MENU_CONTINUE,
    MENU_OPTIONS,
    MENU_COUNT
};

int main(int argc, char* argv[]) {
    // 1. Initialize Hardware Services & RomFS
    gfxInitDefault();
    romfsInit();

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // 2. Create Render Targets for both screens
    C3D_RenderTarget* topTarget    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // 3. Load Compiled Sprite Sheet from RomFS (compiled from gfx/sprites.t3s)
    C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");

    Player player;
    int selectedOption = MENU_START;

    // --- MAIN GAME LOOP ---
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break; // Exit game on START button

        // Reset movement status for this frame
        player.isMoving = false;

        // --- CIRCLE PAD & D-PAD INPUT LOGIC ---
        circlePosition circle;
        hidCircleRead(&circle);

        if (circle.dy < -15 || (kHeld & KEY_DDOWN)) {
            player.y += player.speed;
            player.dir = DIR_DOWN;
            player.isMoving = true;
        } 
        else if (circle.dy > 15 || (kHeld & KEY_DUP)) {
            player.y -= player.speed;
            player.dir = DIR_UP;
            player.isMoving = true;
        } 
        else if (circle.dx < -15 || (kHeld & KEY_DLEFT)) {
            player.x -= player.speed;
            player.dir = DIR_LEFT;
            player.isMoving = true;
        } 
        else if (circle.dx > 15 || (kHeld & KEY_DRIGHT)) {
            player.x += player.speed;
            player.dir = DIR_RIGHT;
            player.isMoving = true;
        }

        // --- TOP SCREEN BOUNDARY CLAMP ---
        if (player.x < 0) player.x = 0;
        if (player.x > 384) player.x = 384;
        if (player.y < 0) player.y = 0;
        if (player.y > 224) player.y = 224;

        // --- SPRITE FRAME ANIMATION LOGIC ---
        if (player.isMoving) {
            player.animTimer++;
            if (player.animTimer >= 8) { // Advance frame every 8 ticks
                player.animTimer = 0;
                player.currentFrame = (player.currentFrame + 1) % 4; // Cycle 0->1->2->3
            }
        } else {
            player.currentFrame = 0; // Standing idle frame
        }

        // --- BOTTOM SCREEN MENU NAVIGATION ---
        if (kDown & KEY_CSTICK_DOWN) {
            selectedOption = (selectedOption + 1) % MENU_COUNT;
        } 
        else if (kDown & KEY_CSTICK_UP) {
            selectedOption = (selectedOption == 0) ? (MENU_COUNT - 1) : (selectedOption - 1);
        }

        // --- RENDER FRAME ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // ==========================================
        // 1. TOP SCREEN (Gameplay World)
        // ==========================================
        C2D_TargetClear(topTarget, C2D_Color32(0x30, 0x60, 0x30, 0xFF)); // Green background
        C2D_SceneBegin(topTarget);

        if (spriteSheet) {
            // Calculate 4x4 sprite sheet index: (row * 4) + column
            int spriteIndex = (player.dir * 4) + player.currentFrame;
            C2D_Image playerSprite = C2D_SpriteSheetGetImage(spriteSheet, spriteIndex);
            C2D_DrawImageAt(playerSprite, player.x, player.y, 0.5f, NULL, 1.0f, 1.0f);
        } else {
            // Red placeholder rectangle if spritesheet load fails
            C2D_DrawRectSolid(player.x, player.y, 0.5f, 16.0f, 16.0f, C2D_Color32(255, 0, 0, 255));
        }

        // ==========================================
        // 2. BOTTOM SCREEN (Interactive Menu)
        // ==========================================
        C2D_TargetClear(bottomTarget, C2D_Color32(0x15, 0x15, 0x15, 0xFF)); // Dark gray UI
        C2D_SceneBegin(bottomTarget);

        // Render UI menu selection blocks
        C2D_DrawRectSolid(80.0f, 40.0f, 0.5f, 160.0f, 30.0f, 
            (selectedOption == MENU_START) ? C2D_Color32(200, 200, 50, 255) : C2D_Color32(80, 80, 80, 255));

        C2D_DrawRectSolid(80.0f, 95.0f, 0.5f, 160.0f, 30.0f, 
            (selectedOption == MENU_CONTINUE) ? C2D_Color32(200, 200, 50, 255) : C2D_Color32(80, 80, 80, 255));

        C2D_DrawRectSolid(80.0f, 150.0f, 0.5f, 160.0f, 30.0f, 
            (selectedOption == MENU_OPTIONS) ? C2D_Color32(200, 200, 50, 255) : C2D_Color32(80, 80, 80, 255));

        C3D_FrameEnd(0);
    }

    // Clean up hardware resources on exit
    if (spriteSheet) C2D_SpriteSheetFree(spriteSheet);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
