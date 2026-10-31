#include <3ds.h>
#include <citro2d.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>
#include <cstdio>

// Include your custom project modules
#include "player.h"
#include "world.h"
#include "ItemTable.h"
#include "ShaderMenu.h"

// Game States
enum GameState {
    STATE_CONSOLE_DEBUG,
    STATE_MENU,
    STATE_PLAYING
};

// Global UI and Storage
std::unordered_map<std::string, std::string> g_uiStrings;
std::unordered_map<std::string, std::string> g_gameLogs;
nlohmann::json g_mainGameData; 
C2D_TextBuf g_dynamicTextBuf;
C2D_SpriteSheet g_spriteSheet;
C2D_Sprite g_playerSprite;

// Helper function to read a file in chunks and show live percentage progress
bool loadFileWithProgress(const std::string& filepath, std::string& dest, int basePercent, int maxPercent) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    dest.resize(fileSize);
    size_t bytesRead = 0;
    char buffer[65536]; // 64KB chunks

    while (bytesRead < fileSize && file) {
        size_t toRead = std::min(sizeof(buffer), fileSize - bytesRead);
        file.read(buffer, toRead);
        std::memcpy(&dest[bytesRead], buffer, toRead);
        bytesRead += toRead;

        // Calculate smooth live percentage between base and max bounds
        int progress = basePercent + ((maxPercent - basePercent) * bytesRead / fileSize);
        printf("\r[%3d%%] Loading file...      ", progress);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
    return true;
}

// Merged game data loader using the live chunk progress tracker
bool loadMergedGameData(nlohmann::json& targetJson) {
    std::string part1, part2;

    // Load Part 1 (scales smoothly from 0% to 45%)
    if (!loadFileWithProgress("romfs:/data_part_1.json", part1, 0, 45)) {
        printf("[ERROR] Failed to open data_part_1.json!\n");
        fflush(stdout);
        return false;
    }

    // Load Part 2 (scales smoothly from 45% to 90%)
    if (!loadFileWithProgress("romfs:/data_part_2.json", part2, 45, 90)) {
        printf("[ERROR] Failed to open data_part_2.json!\n");
        fflush(stdout);
        return false;
    }

    printf("[95%] Merging & Parsing JSON data...\n");
    fflush(stdout);

    std::string mergedJsonString = part1 + part2;
    
    // Parse safely with allow_exceptions = false (compatible with 3DS devkitPro settings)
    targetJson = nlohmann::json::parse(mergedJsonString, nullptr, false);

    if (targetJson.is_discarded()) {
        printf("[ERROR] JSON parse failed (discarded)!\n");
        fflush(stdout);
        return false; 
    }

    printf("[100%] Game data loaded and parsed successfully!\n");
    fflush(stdout);
    return true;
}

// Helper function to parse JSON string maps
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
    // 1. Initialize 3DS Hardware Services & Console
    gfxInitDefault();
    
    PrintConsole bottomConsole;
    consoleInit(GFX_BOTTOM, &bottomConsole); // Initialize debug text on bottom screen
    
    printf("=== MiniDayZ 3DS Booting ===\n\n");

    printf("[INIT] Initializing RomFS...\n");
    fflush(stdout);
    if (romfsInit() != 0) {
        printf("[ERROR] romfsInit() failed! Are assets packed?\n");
        fflush(stdout);
    } else {
        printf("[SUCCESS] RomFS initialized.\n");
        fflush(stdout);
    }

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets for Top and Bottom screens
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Initialize Citro2D Dynamic Text Buffer
    g_dynamicTextBuf = C2D_TextBufNew(1024);

    // 2. Load Split Main Game Data Chunks with live percentage tracking
    bool dataLoaded = loadMergedGameData(g_mainGameData);

    // Load Secondary UI and Game Log Data
    loadJsonStrings("romfs:/data/ui_strings.json", g_uiStrings);
    loadJsonStrings("romfs:/data/game_logs.json", g_gameLogs);
    
    // Initialize Game Modules
    World_LoadData();
    Player_Init();
    ItemTable_Load();
    ShaderMenu_Init();

    // Load compiled sprite sheet from RomFS
    g_spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/player.t3x");
    if (g_spriteSheet) {
        printf("[LOAD] Loaded player.t3x sprite sheet.\n");
        fflush(stdout);
        C2D_SpriteFromSheet(&g_playerSprite, g_spriteSheet, 0);
        C2D_SpriteSetCenter(&g_playerSprite, 0.5f, 0.5f);
    } else {
        printf("[WARNING] Failed to load player.t3x sprite sheet!\n");
        fflush(stdout);
    }

    // Set menu button fallback strings
    std::string newGameText = g_uiStrings.count("1") ? g_uiStrings["1"] : "New Game";
    std::string continueText = g_uiStrings.count("2") ? g_uiStrings["2"] : "Continue";
    std::string activeLog = g_gameLogs.count("4") ? g_gameLogs["4"] : "I'm freezing!";

    printf("\nInitialization complete!\n");
    printf("Press START to launch game.\n");
    fflush(stdout);

    GameState currentState = STATE_CONSOLE_DEBUG;

    // Main Loop
    while (aptMainLoop()) {
        // Scan Input
        hidScanInput();
        u32 kDown = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        // Press START to Exit or transition from debug console
        if (kDown & KEY_START) {
            if (currentState == STATE_CONSOLE_DEBUG) {
                currentState = STATE_MENU; // Switch to graphical menu on button press
            } else {
                break; // Exit app entirely
            }
        }

        if (currentState == STATE_CONSOLE_DEBUG) {
            // Keep rendering the text console on the bottom screen until user presses START
            gspWaitForVBlank();
            continue;
        }

        // Clear dynamic text buffer every frame
        C2D_TextBufClear(g_dynamicTextBuf);

        // --- Touch Input Logic for Menu ---
        if (currentState == STATE_MENU && (kDown & KEY_TOUCH)) {
            // "New Game" Button bounds (x: 40-280, y: 50-90)
            if (touch.px >= 40.0f && touch.px <= 280.0f && touch.py >= 50.0f && touch.py <= 90.0f) {
                currentState = STATE_PLAYING;
            }
            // "Continue" Button bounds (x: 40-280, y: 110-150)
            else if (touch.px >= 40.0f && touch.px <= 280.0f && touch.py >= 110.0f && touch.py <= 150.0f) {
                currentState = STATE_PLAYING;
            }
        }

        // Begin Rendering Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        if (currentState == STATE_MENU) {
            // ==========================================
            // 1. RENDER MENU SCREEN (ShaderMenu Module)
            // ==========================================
            ShaderMenu_DrawBackground(topScreen);

            // Draw status log text on top screen
            C2D_Text logText;
            C2D_TextParse(&logText, g_dynamicTextBuf, activeLog.c_str());
            C2D_TextOptimize(&logText);
            C2D_DrawText(&logText, C2D_AtBaseline | C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 80, 80, 255));

            C2D_TargetClear(bottomScreen, C2D_Color32(40, 40, 40, 255));
            C2D_SceneBegin(bottomScreen);

            // Draw "New Game" Button
            C2D_DrawRectangle(40.0f, 50.0f, 0, 240.0f, 40.0f, 
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255),
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255));
            C2D_Text btn1Text;
            C2D_TextParse(&btn1Text, g_dynamicTextBuf, newGameText.c_str());
            C2D_TextOptimize(&btn1Text);
            C2D_DrawText(&btn1Text, C2D_AtBaseline | C2D_WithColor, 60.0f, 78.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

            // Draw "Continue" Button
            C2D_DrawRectangle(40.0f, 110.0f, 0, 240.0f, 40.0f, 
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255),
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255));
            C2D_Text btn2Text;
            C2D_TextParse(&btn2Text, g_dynamicTextBuf, continueText.c_str());
            C2D_TextOptimize(&btn2Text);
            C2D_DrawText(&btn2Text, C2D_AtBaseline | C2D_WithColor, 60.0f, 138.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

        } else if (currentState == STATE_PLAYING) {
            // ==========================================
            // 2. RENDER GAMEPLAY SCREEN (Player & World)
            // ==========================================
            circlePosition circlePad;
            hidCircleRead(&circlePad);
            Player_Update(circlePad);

            C2D_TargetClear(topScreen, C2D_Color32(10, 40, 10, 255));
            C2D_SceneBegin(topScreen);

            // Render Map Objects from World Module
            const auto& mapObjects = World_GetObjects();
            for (const auto& obj : mapObjects) {
                C2D_DrawRectangle(obj.x, obj.y, 0, 4.0f, 4.0f,
                                  C2D_Color32(0, 220, 0, 255), C2D_Color32(0, 220, 0, 255),
                                  C2D_Color32(0, 200, 0, 255), C2D_Color32(0, 200, 0, 255));
            }

            // Render Player Sprite
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

        // End Frame
        C3D_FrameEnd(0);
    }

    // Cleanup Services
    if (g_spriteSheet) C2D_SpriteSheetFree(g_spriteSheet);
    C2D_TextBufDelete(g_dynamicTextBuf);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
