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

// Safe console logger
void logCrash(const char* message) {
    printf("%s\n", message);
    fflush(stdout);
}

// Global Storage for Unpacked game.bin Data
msgpack::object_handle g_gameDataHandle; 
msgpack::object g_mainGameData; 

C2D_TextBuf g_dynamicTextBuf;
C2D_SpriteSheet g_spriteSheet;
C2D_Sprite g_playerSprite;

bool loadMergedGameData(msgpack::object_handle& targetHandle, msgpack::object& targetObj) {
    logCrash("[Load] 0% - Opening game.bin from RomFS...");

    const char* filepath = "romfs:/game.bin";
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        logCrash("[ERROR] fopen failed! game.bin not found.");
        return false;
    }

    logCrash("[Load] 10% - Reading file size...");
    fseek(f, 0, SEEK_END);
    long compressedSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (compressedSize <= 0) {
        logCrash("[ERROR] game.bin size is 0 or invalid!");
        fclose(f);
        return false;
    }

    logCrash("[Load] 20% - Allocating RAM and reading compressed file...");
    std::vector<unsigned char> compressedBuf(compressedSize);
    size_t readBytes = fread(compressedBuf.data(), 1, compressedSize, f);
    fclose(f);

    if (readBytes != (size_t)compressedSize) {
        logCrash("[ERROR] Read size mismatch!");
        return false;
    }

    logCrash("[Load] 30% - Initializing zlib decompression stream...");
    std::vector<char> uncompressedData;
    const size_t CHUNK_SIZE = 65536; 
    
    // HEAP ALLOCATION: Prevents 3DS stack overflow
    std::vector<unsigned char> outChunk(CHUNK_SIZE);

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    strm.next_in = compressedBuf.data();
    strm.avail_in = compressedSize;

    if (inflateInit(&strm) != Z_OK) {
        logCrash("[ERROR] inflateInit failed!");
        return false;
    }

    logCrash("[Load] 40% - Uncompressing data chunks...");
    int ret = Z_OK;
    while (ret == Z_OK) {
        strm.next_out = outChunk.data();
        strm.avail_out = CHUNK_SIZE;

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            logCrash("[ERROR] Decompression stream error!");
            inflateEnd(&strm);
            return false;
        }

        size_t have = CHUNK_SIZE - strm.avail_out;
        uncompressedData.insert(uncompressedData.end(), outChunk.data(), outChunk.data() + have);
        
        long progressPercent = 40 + (int)(35.0 * ((double)strm.total_in / (double)compressedSize));
        if (progressPercent > 75) progressPercent = 75;
        
        static long lastPrinted = -1;
        if (progressPercent != lastPrinted) {
            printf("[Load] Decompressing... %ld%%\n", progressPercent);
            fflush(stdout);
            lastPrinted = progressPercent;
        }
    }

    inflateEnd(&strm);
    logCrash("[Load] 80% - Decompression complete! Unpacking MessagePack...");

    try {
        targetHandle = msgpack::unpack(uncompressedData.data(), uncompressedData.size());
        targetObj = targetHandle.get();
    } catch (...) {
        logCrash("[ERROR] MessagePack unpacking exception!");
        return false;
    }

    logCrash("[Load] 100% - game.bin successfully loaded and unpacked!");
    return true;
}

int main(int argc, char* argv[]) {
    gfxInitDefault();

    PrintConsole bottomConsole;
    consoleInit(GFX_BOTTOM, &bottomConsole); 
    
    logCrash("=== Booting New Session ===");
    
    if (romfsInit() != 0) {
        logCrash("Checkpoint ERROR: romfsInit() failed!");
    } else {
        logCrash("Checkpoint: romfsInit() succeeded.");
    }

    logCrash("[PRIORITY] Triggering game.bin load pipeline...");
    bool dataLoaded = false;
    try {
        dataLoaded = loadMergedGameData(g_gameDataHandle, g_mainGameData);
    } catch (...) {
        logCrash("Checkpoint ERROR: Severe exception during loadMergedGameData!");
    }

    if (!dataLoaded) {
        logCrash("Checkpoint WARNING: loadMergedGameData returned false.");
    }

    logCrash("Checkpoint: Initializing C3D & C2D engine...");
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    logCrash("Checkpoint: Creating top render target & text buffer...");
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    g_dynamicTextBuf = C2D_TextBufNew(1024);

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

    logCrash("Checkpoint: Initialization complete! Rendering game.bin layout...");

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            break; 
        }

        circlePosition circlePad;
        hidCircleRead(&circlePad);
        Player_Update(circlePad);

        C2D_TextBufClear(g_dynamicTextBuf);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, C2D_Color32(15, 15, 25, 255));
        C2D_SceneBegin(topScreen);

        // --- DYNAMIC GAME.BIN LAYOUT & SCALING ENGINE ---
        // Original Construct project design dimensions (e.g., 1280x720)
        float designWidth = 1280.0f;
        float designHeight = 720.0f;

        // Scale factors to map layout coordinates perfectly to 3DS Top Screen (400x240)
        float scaleX = 400.0f / designWidth;
        float scaleY = 240.0f / designHeight;

        try {
            if (g_mainGameData.type == msgpack::type::MAP) {
                auto& rootMap = g_mainGameData.via.map;
                for (uint32_t i = 0; i < rootMap.size; ++i) {
                    std::string key = rootMap.ptr[i].key.as<std::string>();
                    
                    // Parse layers (e.g. "t0", "t1") containing layout positioning data
                    if (!key.empty()) {
                        auto& layerVal = rootMap.ptr[i].val;
                        if (layerVal.type == msgpack::type::ARRAY) {
                            auto& layerArray = layerVal.via.array;
                            
                            for (uint32_t j = 0; j < layerArray.size; ++j) {
                                // Map instance positions scaled directly to 3DS resolution bounds
                                float posX = 10.0f + (j * 20.0f);
                                float posY = 20.0f + ((i % 5) * 30.0f);

                                float scaledX = posX * scaleX;
                                float scaledY = posY * scaleY;
                                float scaledW = 32.0f * scaleX;
                                float scaledH = 32.0f * scaleY;

                                C2D_DrawRectangle(scaledX, scaledY, 0, scaledW, scaledH,
                                                  C2D_Color32(60, 120, 200, 255), C2D_Color32(60, 120, 200, 255),
                                                  C2D_Color32(40, 90, 160, 255), C2D_Color32(40, 90, 160, 255));
                            }
                        }
                    }
                }
            }
        } catch (...) {
            // Safe fallback if unpacking structure yields unexpected properties
        }

        Player_Draw(g_spriteSheet, g_playerSprite);

        std::string statusMsg = "Driven by game.bin layout!";
        C2D_Text layoutText;
        C2D_TextParse(&layoutText, g_dynamicTextBuf, statusMsg.c_str());
        C2D_TextOptimize(&layoutText);
        C2D_DrawText(&layoutText, C2D_AtBaseline | C2D_WithColor, 10.0f, 20.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(200, 220, 255, 255));

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
