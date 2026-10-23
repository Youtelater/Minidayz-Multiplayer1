#include <3ds.h>
#include <citro2d.h>
#include <ndsp.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "player.h"
#include "world.h"

// Game States
enum GameState {
    STATE_MENU,
    STATE_PLAYING
};

// Global UI and Storage
std::unordered_map<std::string, std::string> g_uiStrings;
std::unordered_map<std::string, std::string> g_gameLogs;
C2D_TextBuf g_dynamicTextBuf;
C2D_SpriteSheet g_spriteSheet;
C2D_Sprite g_playerSprite;

// Helper to parse JSON string maps
bool loadJsonStrings(const char* filePath, std::unordered_map<std::string, std::string>& targetMap) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    nlohmann::json j;
    file >> j;
    for (auto& [key, val] : j.items()) {
        if (val.is_string()) {
            targetMap[key] = val.get<std::string>();
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    // 1. Initialize 3DS Services & Audio
    gfxInitDefault();
    romfsInit();
    ndspInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets for Top and Bottom screens
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Initialize Citro2D Dynamic Text Buffer
    g_dynamicTextBuf = C2D_TextBufNew(1024);

    // 2. Load RomFS Assets & Data via Modules
    loadJsonStrings("romfs:/data/ui_strings.json", g_uiStrings);
    loadJsonStrings("romfs:/data/game_logs.json", g_gameLogs);
    
    World_LoadData();
    Player_Init();

    // Load compiled .t3x sprite sheet from RomFS
    g_spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/player.t3x");
    if (g_spriteSheet) {
        C2D_SpriteFromSheet(&g_playerSprite, g_spriteSheet, 0);
        C2D_SpriteSetCenter(&g_playerSprite, 0.5f, 0.5f);
    }

    // Set fallback strings if keys are missing
    std::string newGameText = g_uiStrings.count("1") ? g_uiStrings["1"] : "New Game";
    std::string continueText = g_uiStrings.count("2") ? g_uiStrings["2"] : "Continue";
    std::string activeLog = g_gameLogs.count("4") ? g_gameLogs["4"] : "I'm freezing!";

    GameState currentState = STATE_MENU;

    // Main Game Loop
    while (aptMainLoop()) {
        // Scan User Input
        hidScanInput();
        u32 kDown = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        // Press START to Exit App
        if (kDown & KEY_START) break;

        // Clear dynamic text buffer every frame before drawing new text
        C2D_TextBufClear(g_dynamicTextBuf);

        // --- Touch Logic for Menu Buttons ---
        if (currentState == STATE_MENU && (kDown & KEY_TOUCH)) {
            // Button 1: "New Game" bounds (x: 40-280, y: 50-90)
            if (touch.px >= 40.0f && touch.px <= 280.0f && touch.py >= 50.0f && touch.py <= 90.0f) {
                currentState = STATE_PLAYING;
            }
            // Button 2: "Continue" bounds (x: 40-280, y: 110-150)
            else if (touch.px >= 40.0f && touch.px <= 280.0f && touch.py >= 110.0f && touch.py <= 150.0f) {
                currentState = STATE_PLAYING;
            }
        }

        // Begin the 3DS frame rendering sequence
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        if (currentState == STATE_MENU) {
            // ==========================================
            // 1. RENDER MENU SCREENS
            // ==========================================
            C2D_TargetClear(topScreen, C2D_Color32(20, 20, 20, 255));
            C2D_SceneBegin(topScreen);

            // Draw status log text on top screen
            C2D_Text logText;
            C2D_TextParse(&logText, g_dynamicTextBuf, activeLog.c_str());
            C2D_TextOptimize(&logText);
            C2D_DrawText(&logText, C2D_AtBaseline | C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 80, 80, 255));

            C2D_TargetClear(bottomScreen, C2D_Color32(40, 40, 40, 255));
            C2D_SceneBegin(bottomScreen);

            // Draw Button 1 ("New Game")
            C2D_DrawRectangle(40.0f, 50.0f, 0, 240.0f, 40.0f, 
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255),
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255));
            C2D_Text btn1Text;
            C2D_TextParse(&btn1Text, g_dynamicTextBuf, newGameText.c_str());
            C2D_TextOptimize(&btn1Text);
            C2D_DrawText(&btn1Text, C2D_AtBaseline | C2D_WithColor, 60.0f, 78.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

            // Draw Button 2 ("Continue")
            C2D_DrawRectangle(40.0f, 110.0f, 0, 240.0f, 40.0f, 
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255),
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255));
            C2D_Text btn2Text;
            C2D_TextParse(&btn2Text, g_dynamicTextBuf, continueText.c_str());
            C2D_TextOptimize(&btn2Text);
            C2D_DrawText(&btn2Text, C2D_AtBaseline | C2D_WithColor, 60.0f, 138.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

        } else if (currentState == STATE_PLAYING) {
            // ==========================================
            // 2. RENDER GAMEPLAY SCREENS
            // ==========================================
            
            // Read Circle Pad Input and Update Player via Module
            circlePosition circlePad;
            hidCircleRead(&circlePad);
            Player_Update(circlePad);

            C2D_TargetClear(topScreen, C2D_Color32(10, 40, 10, 255));
            C2D_SceneBegin(topScreen);

            // Draw Map Objects from World Module
            const auto& mapObjects = World_GetObjects();
            for (const auto& obj : mapObjects) {
                C2D_DrawRectangle(obj.x, obj.y, 0, 4.0f, 4.0f,
                                  C2D_Color32(0, 220, 0, 255), C2D_Color32(0, 220, 0, 255),
                                  C2D_Color32(0, 200, 0, 255), C2D_Color32(0, 200, 0, 255));
            }

            // Draw Player Sprite via Player Module
            Player_Draw(g_spriteSheet, g_playerSprite);

            // Display Active In-Game Log Overlay
            std::string gameplayLog = g_gameLogs.count("2") ? g_gameLogs["2"] : "I feel healthy.";
            C2D_Text liveLogText;
            C2D_TextParse(&liveLogText, g_dynamicTextBuf, gameplayLog.c_str());
            C2D_TextOptimize(&liveLogText);
            C2D_DrawText(&liveLogText, C2D_AtBaseline | C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

            C2D_TargetClear(bottomScreen, C2D_Color32(20, 20, 20, 255));
            C2D_SceneBegin(bottomScreen);
        }

        // End the 3DS frame rendering sequence
        C3D_FrameEnd(0);
    }

    // Deinitialize and Cleanup
    if (g_spriteSheet) C2D_SpriteSheetFree(g_spriteSheet);
    C2D_TextBufDelete(g_dynamicTextBuf);
    C2D_Fini();
    C3D_Fini();
    ndspExit();
    romfsExit();
    gfxExit();
    return 0;
}
