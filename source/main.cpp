#include <3ds.h>
#include <citro2d.h>

enum GameState {
    STATE_SPLASH,
    STATE_LOADING,
    STATE_MENU,
    STATE_GAMEPLAY
};

enum SpriteIndex {
    SPRITE_PLAYER = 0,
    SPRITE_LOADING_LOGO,
    SPRITE_MENU_SKY,
    SPRITE_MENU_SILHOUETTE,
    SPRITE_MENU_LOGO
};

int main(int argc, char* argv[]) {
    // Initialize graphics and services
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    romfsInit();

    // Create render targets for dual 3DS screens
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Load packed texture sheet from RomFS
    C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    
    // Extract individual images by atlas index
    C2D_Image imgSplashLogo = C2D_SpriteSheetGetImage(spriteSheet, SPRITE_LOADING_LOGO);
    C2D_Image imgMenuSky    = C2D_SpriteSheetGetImage(spriteSheet, SPRITE_MENU_SKY);
    C2D_Image imgMenuSil    = C2D_SpriteSheetGetImage(spriteSheet, SPRITE_MENU_SILHOUETTE);
    C2D_Image imgMenuLogo   = C2D_SpriteSheetGetImage(spriteSheet, SPRITE_MENU_LOGO);

    GameState state = STATE_SPLASH;
    int splashTimer = 0;
    float loadingProgress = 0.0f;

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        // Press START anywhere to exit
        if (kDown & KEY_START) break;

        // --- State Machine Updates ---
        if (state == STATE_SPLASH) {
            splashTimer++;
            // Show splash logo for ~2 seconds (120 frames at 60fps) or skip with A
            if (splashTimer >= 120 || (kDown & KEY_A)) {
                state = STATE_LOADING;
            }
        } 
        else if (state == STATE_LOADING) {
            loadingProgress += 0.015f; // Fills progress bar
            if (loadingProgress >= 1.0f) {
                loadingProgress = 1.0f;
                state = STATE_MENU;
            }
        } 
        else if (state == STATE_MENU) {
            if (kDown & KEY_A) {
                state = STATE_GAMEPLAY; // Press A to start game
            }
        }

        // --- Render Top Screen (400x240) ---
        C2D_SceneBegin(top);
        C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));

        if (state == STATE_SPLASH) {
            // Center Bohemia Interactive logo
            float x = (400.0f - imgSplashLogo.subtex->width) / 2.0f;
            float y = (240.0f - imgSplashLogo.subtex->height) / 2.0f;
            C2D_DrawImageAt(imgSplashLogo, x, y, 0.5f, NULL, 1.0f, 1.0f);
        } 
        else if (state == STATE_LOADING || state == STATE_MENU) {
            // Layer 1: Stretched Sky background pinned to top (displays upper red section)
            float scaleX = 400.0f / imgMenuSky.subtex->width;
            float scaleY = 240.0f / imgMenuSky.subtex->height;
            C2D_DrawImageAt(imgMenuSky, 0.0f, 0.0f, 0.1f, NULL, scaleX, scaleY * 2.0f);

            // Layer 2: City Silhouette aligned to bottom edge
            float silY = 240.0f - imgMenuSil.subtex->height;
            float silScaleX = 400.0f / imgMenuSil.subtex->width;
            C2D_DrawImageAt(imgMenuSil, 0.0f, silY, 0.2f, NULL, silScaleX, 1.0f);

            // Layer 3: Mini DAYZ Logo centered horizontally
            float logoX = (400.0f - imgMenuLogo.subtex->width) / 2.0f;
            float logoY = 60.0f;
            C2D_DrawImageAt(imgMenuLogo, logoX, logoY, 0.3f, NULL, 1.0f, 1.0f);
        } 
        else if (state == STATE_GAMEPLAY) {
            C2D_TargetClear(top, C2D_Color32(34, 139, 34, 255));
            // Player and map rendering logic goes here
        }

        // --- Render Bottom Screen (320x240) ---
        C2D_SceneBegin(bottom);
        C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));

        if (state == STATE_LOADING) {
            // Centered loading bar parameters
            float barWidth = 240.0f;
            float barHeight = 8.0f;
            float barX = (320.0f - barWidth) / 2.0f;
            float barY = 200.0f;

            // Outer gray track
            C2D_DrawRectSolid(barX, barY, 0.5f, barWidth, barHeight, C2D_Color32(80, 80, 80, 255));
            // Inner white progress fill
            C2D_DrawRectSolid(barX, barY, 0.6f, barWidth * loadingProgress, barHeight, C2D_Color32(220, 220, 220, 255));
        }

        C3D_FrameEnd(0);
    }

    // Cleanup resources
    C2D_SpriteSheetFree(spriteSheet);
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
