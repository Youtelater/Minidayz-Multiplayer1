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

// Screen logger: prints directly to the emulator console
void logCrash(const char* message) {
    printf("%s\n", message);
    fflush(stdout);
}

// Game States
enum GameState {
    STATE_MENU,
    STATE_PLAYING
};

// Global UI and Storage
std::unordered_map<std::string, std::string> g_uiStrings;
std::unordered_map<std::string, std::string> g_gameLogs;

msgpack::object_handle g_gameDataHandle; 
msgpack::object g_mainGameData; 

C2D_TextBuf g_dynamicTextBuf;
C2D_SpriteSheet g_spriteSheet;
C2D_Sprite g_playerSprite;

bool loadMergedGameData(msgpack::object_handle& targetHandle, msgpack::object& targetObj) {
    logCrash("[Load] 0% - Entering load sequence...");
    gspWaitForVBlank();

    const char* filepath = "romfs:/game.bin";
    logCrash("[Load] 5% - Opening game.bin...");
    
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        logCrash("[ERROR] 5% - fopen returned nullptr! File missing from RomFS?");
        return false;
    }

    logCrash("[Load] 10% - Reading file size...");
    fseek(f, 0, SEEK_END);
    long compressedSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (compressedSize <= 0) {
        logCrash("[ERROR] 10% - game.bin size is 0 or invalid!");
        fclose(f);
        return false;
    }

    logCrash("[Load] 15% - Allocating memory for compressed buffer...");
    std::vector<unsigned char> compressedBuf(compressedSize);
    
    logCrash("[Load] 20% - Reading data into buffer...");
    size_t readBytes = fread(compressedBuf.data(), 1, compressedSize, f);
    fclose(f);

    if (readBytes != (size_t)compressedSize) {
        logCrash("[ERROR] Read size mismatch!");
        return false;
    }

    logCrash("[Load] 30% - Initializing zlib stream...");
    std::vector<char> uncompressedData;
    const size_t CHUNK_SIZE = 65536; 
    unsigned char outChunk[CHUNK_SIZE];

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    strm.next_in = compressedBuf.data();
    strm.avail_in = compressedSize;

    if (inflateInit(&strm) != Z_OK) {
        logCrash("[ERROR] inflateInit failed!");
        return false;
    }

    logCrash("[Load] 40% - Beginning zlib chunk decompression...");
    int ret = Z_OK;
    int chunkCount = 0;
    while (ret == Z_OK) {
        strm.next_out = outChunk;
        strm.avail_out = CHUNK_SIZE;

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            logCrash("[ERROR] Decompression error during inflate loop!");
            inflateEnd(&strm);
            return false;
        }

        size_t have = CHUNK_SIZE - strm.avail_out;
        uncompressedData.insert(uncompressedData.end(), outChunk, outChunk + have);
        
        chunkCount++;
        if (chunkCount % 5 == 0) {
            printf("[Load] Decompressing chunk %d...\n", chunkCount);
            gfxFlushBuffers();
        }
    }

    inflateEnd(&strm);
    logCrash("[Load] 70% - Decompression successful! Preparing MessagePack...");

    try {
        logCrash("[Load] 80% - Unpacking MessagePack binary data...");
        targetHandle = msgpack::unpack(uncompressedData.data(), uncompressedData.size());
        logCrash("[Load] 90% - Getting MessagePack object handle...");
        targetObj = targetHandle.get();
    } catch (const std::exception& e) {
        logCrash("[ERROR] MessagePack unpack threw an exception!");
        return false;
    }

    logCrash("[Load] 100% - Game data fully loaded and unpacked!");
    return true;
}

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
    gfxInitDefault();

    logCrash("=== Booting New Session ===");
    logCrash("Checkpoint: gfxInitDefault passed.");
    
    PrintConsole bottomConsole;
    consoleInit(GFX_BOTTOM, &bottomConsole); 
    logCrash("Checkpoint: consoleInit passed.");
    
    logCrash("Checkpoint: About to call romfsInit()...");
    if (romfsInit() != 0) {
        logCrash("Checkpoint ERROR: romfsInit() failed!");
    } else {
        logCrash("Checkpoint: romfsInit() succeeded.");
    }

    logCrash("Checkpoint: Initializing C3D & C2D...");
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    logCrash("Checkpoint: Creating top render target & text buffer...");
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    g_dynamicTextBuf = C2D_TextBufNew(1024);

    logCrash("Checkpoint: About to trigger file load sequence...");
    
    gfxFlushBuffers();
    gspWaitForVBlank();

    bool dataLoaded = false;
    try {
        dataLoaded = loadMergedGameData(g_gameDataHandle, g_mainGameData);
    } catch (...) {
        logCrash("Checkpoint ERROR: Caught severe exception during loadMergedGameData!");
    }

    if (!dataLoaded) {
        logCrash("Checkpoint WARNING: loadMergedGameData returned false.");
    }

    logCrash("Checkpoint: Loading UI JSON strings...");
    loadJsonStrings("romfs:/data/ui_strings.json", g_uiStrings);
    loadJsonStrings("romfs:/data/game_logs.json", g_gameLogs);
    
    logCrash("Checkpoint: Initializing game modules...");
    World_LoadData();
    Player_Init();
    ItemTable_Load();
    ShaderMenu_Init();

    logCrash("Checkpoint: Loading sprite sheet...");
    g_spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/player.t3x");
    if (g_spriteSheet) {
        logCrash("Checkpoint: player.t3x loaded successfully.");
        C2D_SpriteFromSheet(&g_playerSprite, g_spriteSheet, 0);
        C2D_SpriteSetCenter(&g_playerSprite, 0.5f, 0.5f);
    } else {
        logCrash("Checkpoint WARNING: Failed to load player.t3x sprite sheet!");
    }

    logCrash("Checkpoint: Initialization sequence fully complete!");
    logCrash("Launching straight into the game menu...");

    std::string newGameText = g_uiStrings.count("1") ? g_uiStrings["1"] : "New Game";
    std::string continueText = g_uiStrings.count("2") ? g_uiStrings["2"] : "Continue";
    std::string activeLog = g_gameLogs.count("4") ? g_gameLogs["4"] : "I'm freezing!";

    // Automatically initialize bottom screen render target and enter menu mode
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    GameState currentState = STATE_MENU;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        if (kDown & KEY_START) {
            break; // Press START to quit the app
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
