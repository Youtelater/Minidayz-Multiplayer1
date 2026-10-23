#include <3ds.h>
#include <citro2d.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <nlohmann/json.hpp>

// --- Data Structures ---
struct MapObject {
    float x;
    float y;
};

// Global Data Storage
std::vector<MapObject> g_mapObjects;
std::unordered_map<std::string, std::string> g_uiStrings;
std::unordered_map<std::string, std::string> g_gameLogs;

// Global Text Buffer for Citro2D
C2D_TextBuf g_dynamicTextBuf;

// Helper to parse JSON string maps
bool loadJsonStrings(const char* filePath, std::unordered_map<std::string, std::string>& targetMap) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    nlohmann::json j;
    try {
        file >> j;
        for (auto& [key, val] : j.items()) {
            if (val.is_string()) {
                targetMap[key] = val.get<std::string>();
            }
        }
    } catch (...) {
        return false;
    }
    return true;
}

// Load coordinates from game data
void loadMapData() {
    std::ifstream file("romfs:/data/extracted_audio_and_coords.json");
    if (!file.is_open()) return;

    nlohmann::json j;
    try {
        file >> j;
        if (j.contains("sample_coordinates") && j["sample_coordinates"].is_array()) {
            for (const auto& coord : j["sample_coordinates"]) {
                if (coord.is_array() && coord.size() >= 2) {
                    MapObject obj;
                    obj.x = coord[0].get<float>();
                    obj.y = coord[1].get<float>();
                    g_mapObjects.push_back(obj);
                }
            }
        }
    } catch (...) {}
}

int main(int argc, char* argv[]) {
    // 1. Initialize 3DS Services
    gfxInitDefault();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets for Top and Bottom screens
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Initialize Citro2D Dynamic Text Buffer ( capacity: 1024 characters )
    g_dynamicTextBuf = C2D_TextBufNew(1024);

    // 2. Load RomFS Assets
    loadJsonStrings("romfs:/data/ui_strings.json", g_uiStrings);
    loadJsonStrings("romfs:/data/game_logs.json", g_gameLogs);
    loadMapData();

    // Set fallback strings if keys are missing
    std::string newGameText = g_uiStrings.count("1") ? g_uiStrings["1"] : "New Game";
    std::string continueText = g_uiStrings.count("2") ? g_uiStrings["2"] : "Continue";
    std::string activeLog = g_gameLogs.count("4") ? g_gameLogs["4"] : "I'm freezing!";

    // Main Game Loop
    while (aptMainLoop()) {
        // Scan User Input
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        touchPosition touch;
        hidTouchRead(&touch);

        // Press START to Exit App
        if (kDown & KEY_START) break;

        // Clear dynamic text buffer every frame before drawing new text
        C2D_TextBufClear(g_dynamicTextBuf);

        // ==========================================
        // 1. RENDER TOP SCREEN (Map & Status Log)
        // ==========================================
        C2D_TargetClear(topScreen, C2D_Color32(20, 20, 20, 255));
        C2D_SceneBegin(topScreen);

        // Draw Map Objects
        for (const auto& obj : g_mapObjects) {
            C2D_DrawRectangle(obj.x, obj.y, 0, 4.0f, 4.0f,
                              C2D_Color32(0, 220, 0, 255),
                              C2D_Color32(0, 220, 0, 255),
                              C2D_Color32(0, 200, 0, 255),
                              C2D_Color32(0, 200, 0, 255));
        }

        // Draw Player Status Log Overlay (Top Left)
        C2D_Text logText;
        C2D_TextParse(&logText, g_dynamicTextBuf, activeLog.c_str());
        C2D_TextOptimize(&logText);
        C2D_DrawText(&logText, C2D_AtBaseline | C2D_WithColor, 10.0f, 30.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 80, 80, 255));

        // ==========================================
        // 2. RENDER BOTTOM SCREEN (Touch Menu UI)
        // ==========================================
        C2D_TargetClear(bottomScreen, C2D_Color32(40, 40, 40, 255));
        C2D_SceneBegin(bottomScreen);

        // --- Button 1: "New Game" ---
        float b1X = 40.0f, b1Y = 50.0f, b1W = 240.0f, b1H = 40.0f;
        bool b1Touched = (kDown & KEY_TOUCH) &&
                         (touch.px >= b1X && touch.px <= b1X + b1W &&
                          touch.py >= b1Y && touch.py <= b1Y + b1H);

        u32 b1Color = b1Touched ? C2D_Color32(100, 180, 100, 255) : C2D_Color32(70, 70, 70, 255);
        C2D_DrawRectangle(b1X, b1Y, 0, b1W, b1H, b1Color, b1Color, b1Color, b1Color);

        C2D_Text btn1Text;
        C2D_TextParse(&btn1Text, g_dynamicTextBuf, newGameText.c_str());
        C2D_TextOptimize(&btn1Text);
        C2D_DrawText(&btn1Text, C2D_AtBaseline | C2D_WithColor, b1X + 20.0f, b1Y + 28.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

        // --- Button 2: "Continue" ---
        float b2X = 40.0f, b2Y = 110.0f, b2W = 240.0f, b2H = 40.0f;
        bool b2Touched = (kDown & KEY_TOUCH) &&
                         (touch.px >= b2X && touch.px <= b2X + b2W &&
                          touch.py >= b2Y && touch.py <= b2Y + b2H);

        u32 b2Color = b2Touched ? C2D_Color32(100, 180, 100, 255) : C2D_Color32(70, 70, 70, 255);
        C2D_DrawRectangle(b2X, b2Y, 0, b2W, b2H, b2Color, b2Color, b2Color, b2Color);

        C2D_Text btn2Text;
        C2D_TextParse(&btn2Text, g_dynamicTextBuf, continueText.c_str());
        C2D_TextOptimize(&btn2Text);
        C2D_DrawText(&btn2Text, C2D_AtBaseline | C2D_WithColor, b2X + 20.0f, b2Y + 28.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

        // Render Frame
        C3D_FrameRender();
    }

    // Deinitialize and Cleanup
    C2D_TextBufDelete(g_dynamicTextBuf);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
