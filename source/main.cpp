#include <3ds.h>
#include <citro2d.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <zlib.h>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>

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

// MessagePack storage handles (keeps the unpacked memory alive during runtime)
msgpack::object_handle g_gameDataHandle; 
msgpack::object g_mainGameData; 

C2D_TextBuf g_dynamicTextBuf;
C2D_SpriteSheet g_spriteSheet;
C2D_Sprite g_playerSprite;

// Reads, decompresses zlib payload from game.bin, and unpacks MessagePack binary data safely
bool loadMergedGameData(msgpack::object_handle& targetHandle, msgpack::object& targetObj) {
   const char* filepath = "romfs:/nonexistent.bin";
    printf("[DEBUG] Checking file status...\n");
    fflush(stdout);

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        printf("[ERROR] Failed to open %s\n", filepath);
        fflush(stdout);
        return false;
    }

    printf("[DEBUG] File opened successfully! Getting size...\n");
    fflush(stdout);

    fseek(f, 0, SEEK_END);
    long compressedSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (compressedSize <= 0) {
        printf("[ERROR] game.bin size is 0 or invalid!\n");
        fclose(f);
        return false;
    }

    printf("[LOAD] Size: %ld bytes. Reading into memory...\n", compressedSize);
    fflush(stdout);

    std::vector<unsigned char> compressedBuf(compressedSize);
    size_t readBytes = fread(compressedBuf.data(), 1, compressedSize, f);
    fclose(f);

    if (readBytes != (size_t)compressedSize) {
        printf("[ERROR] Read mismatch!\n");
        return false;
    }

    printf("[SUCCESS] Read complete. Decompressing...\n");
    fflush(stdout);

    // Decompress zlib stream using chunked / streaming inflate
    std::vector<char> uncompressedData;
    const size_t CHUNK_SIZE = 65536; 
    unsigned char outChunk[CHUNK_SIZE];

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    strm.next_in = compressedBuf.data();
    strm.avail_in = compressedSize;

    if (inflateInit(&strm) != Z_OK) {
        printf("[ERROR] inflateInit failed!\n");
        return false;
    }

    int ret = Z_OK;
    while (ret == Z_OK) {
        strm.next_out = outChunk;
        strm.avail_out = CHUNK_SIZE;

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            printf("[ERROR] Decompression error: %d\n", ret);
            inflateEnd(&strm);
            return false;
        }

        size_t have = CHUNK_SIZE - strm.avail_out;
        uncompressedData.insert(uncompressedData.end(), outChunk, outChunk + have);
    }

    inflateEnd(&strm);

    printf("[SUCCESS] Decompressed %zu bytes! Unpacking MessagePack...\n", uncompressedData.size());
    fflush(stdout);

    try {
        targetHandle = msgpack::unpack(uncompressedData.data(), uncompressedData.size());
        targetObj = targetHandle.get();
    } catch (const std::exception& e) {
        printf("[ERROR] MessagePack unpack failed: %s\n", e.what());
        fflush(stdout);
        return false;
    }

    printf("[SUCCESS] Game data fully loaded!\n");
    fflush(stdout);
    return true;
}

// Helper function to parse JSON string maps (for smaller UI files)
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
    consoleInit(GFX_BOTTOM, &bottomConsole); 
    
    printf("=== MiniDayZ 3DS Booting ===\n\n");
    fflush(stdout);

    printf("[INIT] Initializing RomFS...\n");
    fflush(stdout);
    if (romfsInit() != 0) {
        printf("[ERROR] romfsInit() failed! Are assets packed?\n");
        fflush(stdout);
    } else {
        printf("[SUCCESS] RomFS initialized.\n");
        fflush(stdout);
    }

    printf("[DEBUG] Initializing C3D...\n");
    fflush(stdout);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    printf("[DEBUG] Initializing C2D...\n");
    fflush(stdout);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    printf("[DEBUG] Creating render targets...\n");
    fflush(stdout);
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    printf("[DEBUG] Allocating text buffer...\n");
    fflush(stdout);
    g_dynamicTextBuf = C2D_TextBufNew(1024);

    // 2. Load merged binary dataset (game.bin)
    printf("[DEBUG] About to load merged game data...\n");
    fflush(stdout);
    bool dataLoaded = loadMergedGameData(g_gameDataHandle, g_mainGameData);
    if (!dataLoaded) {
        printf("[WARNING] Failed to load main game data!\n");
        fflush(stdout);
    }

    // Load Secondary UI and Game Log Data
    printf("[DEBUG] Loading UI JSON strings...\n");
    fflush(stdout);
    loadJsonStrings("romfs:/data/ui_strings.json", g_uiStrings);
    loadJsonStrings("romfs:/data/game_logs.json", g_gameLogs);
    
    // Initialize Game Modules
    printf("[DEBUG] Initializing game modules...\n");
    fflush(stdout);
    World_LoadData();
    Player_Init();
    ItemTable_Load();
    ShaderMenu_Init();

    printf("[DEBUG] Loading sprite sheet...\n");
    fflush(stdout);
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

    std::string newGameText = g_uiStrings.count("1") ? g_uiStrings["1"] : "New Game";
    std::string continueText = g_uiStrings.count("2") ? g_uiStrings["2"] : "Continue";
    std::string activeLog = g_gameLogs.count("4") ? g_gameLogs["4"] : "I'm freezing!";

    printf("\nInitialization complete!\n");
    printf("Press START to launch game.\n");
    fflush(stdout);

    GameState currentState = STATE_CONSOLE_DEBUG;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        if (kDown & KEY_START) {
            if (currentState == STATE_CONSOLE_DEBUG) {
                currentState = STATE_MENU; 
            } else {
                break; 
            }
        }

        if (currentState == STATE_CONSOLE_DEBUG) {
            gspWaitForVBlank();
            continue;
        }

        C2D_TextBufClear(g_dynamicTextBuf);

        if (currentState == STATE_MENU && (kDown & KEY_TOUCH)) {
            if (touch.px >= 40.0f && touch.px <= 280.0f && touch.py >= 50.0f && touch.py <= 90.0f) {
                currentState = STATE_PLAYING;
            }
            else if (touch.px >= 40.0f && touch.px <= 280.0f && touch.py >= 110.0f && touch.py <= 150.0f) {
                currentState = STATE_PLAYING;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        if (currentState == STATE_MENU) {
            ShaderMenu_DrawBackground(topScreen);

            C2D_Text logText;
            C2D_TextParse(&logText, g_dynamicTextBuf, activeLog.c_str());
            C2D_TextOptimize(&logText);
            C2D_DrawText(&logText, C2D_AtBaseline | C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 80, 80, 255));

            C2D_TargetClear(bottomScreen, C2D_Color32(40, 40, 40, 255));
            C2D_SceneBegin(bottomScreen);

            C2D_DrawRectangle(40.0f, 50.0f, 0, 240.0f, 40.0f, 
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255),
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255));
            C2D_Text btn1Text;
            C2D_TextParse(&btn1Text, g_dynamicTextBuf, newGameText.c_str());
            C2D_TextOptimize(&btn1Text);
            C2D_DrawText(&btn1Text, C2D_AtBaseline | C2D_WithColor, 60.0f, 78.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

            C2D_DrawRectangle(40.0f, 110.0f, 0, 240.0f, 40.0f, 
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255),
                              C2D_Color32(70, 70, 70, 255), C2D_Color32(70, 70, 70, 255));
            C2D_Text btn2Text;
            C2D_TextParse(&btn2Text, g_dynamicTextBuf, continueText.c_str());
            C2D_TextOptimize(&btn2Text);
            C2D_DrawText(&btn2Text, C2D_AtBaseline | C2D_WithColor, 60.0f, 138.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

        } else if (currentState == STATE_PLAYING) {
            circlePosition circlePad;
            hidCircleRead(&circlePad);
            Player_Update(circlePad);

            C2D_TargetClear(topScreen, C2D_Color32(10, 40, 10, 255));
            C2D_SceneBegin(topScreen);

            const auto& mapObjects = World_GetObjects();
            for (const auto& obj : mapObjects) {
                C2D_DrawRectangle(obj.x, obj.y, 0, 4.0f, 4.0f,
                                  C2D_Color32(0, 220, 0, 255), C2D_Color32(0, 220, 0, 255),
                                  C2D_Color32(0, 200, 0, 255), C2D_Color32(0, 200, 0, 255));
            }

            Player_Draw(g_spriteSheet, g_playerSprite);

            std::string gameplayLog = g_gameLogs.count("2") ? g_gameLogs["2"] : "I feel healthy.";
            C2D_Text liveLogText;
            C2D_TextParse(&liveLogText, g_dynamicTextBuf, gameplayLog.c_str());
            C2D_TextOptimize(&liveLogText);
            C2D_DrawText(&liveLogText, C2D_AtBaseline | C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

            C2D_TargetClear(bottomScreen, C2D_Color32(20, 20, 20, 255));
            C2D_SceneBegin(bottomScreen);
        }

        C3D_FrameEnd(0);
    }

    if (g_spriteSheet) C2D_SpriteSheetFree(g_spriteSheet);
    C2D_TextBufDelete(g_dynamicTextBuf);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
