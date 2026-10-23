#include <citro2d.h>
#include <3ds.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>

// Struct for map objects / coordinates
struct MapObject {
    int x;
    int y;
};

// Global Data Storage
std::vector<MapObject> g_mapObjects;
std::vector<std::string> g_audioFiles;
std::map<std::string, std::string> g_localizedStrings; // From your decoded strings JSON
// Note: SoundEffect struct or type depends on your audio implementation (e.g., ndsp-based)

// Helper function to swap .ogg/.m4a extensions to .wav
std::string getWavFilename(const std::string& originalName) {
    std::string name = originalName;
    size_t dotPos = name.find_last_of('.');
    if (dotPos != std::string::npos) {
        name = name.substr(0, dotPos) + ".wav";
    }
    return name;
}

// Function to load all audio files listed in your extracted JSON
void loadAllGameAudio() {
    std::ifstream file("romfs:/data/extracted_audio_and_coords.json");
    if (!file.is_open()) return;

    nlohmann::json j;
    file >> j;

    // 1. Load Audio File References
    if (j.contains("audio_files") && j["audio_files"].is_array()) {
        g_audioFiles = j["audio_files"].get<std::vector<std::string>>();
        
        for (const auto& audioName : g_audioFiles) {
            std::string wavName = getWavFilename(audioName);
            std::string fullPath = "romfs:/audio/" + wavName;
            
            // Example hook: loadWavFromRomfs(fullPath.c_str());
        }
    }

    // 2. Load Coordinates
    if (j.contains("sample_coordinates") && j["sample_coordinates"].is_array()) {
        for (const auto& coord : j["sample_coordinates"]) {
            if (coord.is_array() && coord.size() >= 2) {
                MapObject obj;
                obj.x = std::stoi(coord[0].get<std::string>());
                obj.y = std::stoi(coord[1].get<std::string>());
                g_mapObjects.push_back(obj);
            }
        }
    }
}

// Function to load decoded strings
void loadAndCacheStrings() {
    std::ifstream file("romfs:/data/decoded_strings.json");
    if (!file.is_open()) return;

    nlohmann::json j;
    file >> j;
    // Parse strings into g_localizedStrings if needed
}

int main(int argc, char* argv[]) {
    // 1. Initialize 3DS hardware systems
    gfxInitDefault();
    romfsInit();
    ndspInit();
    consoleInit(GFX_BOTTOM, NULL); // Optional debug console if needed

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Setup Render Targets for Top and Bottom Screens
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // 2. Load Game Data (Strings, Coordinates, and Audio manifests)
    loadAndCacheStrings();
    loadAllGameAudio();

    // Game state tracking
    touchPosition touch;

    // 3. Main game loop
    while (aptMainLoop()) {
        // Handle User Input / Touch
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        if (kDown & KEY_START) break; // Exit to Homebrew menu on START

        hidTouchRead(&touch);
        if (kDown & KEY_TOUCH) {
            // Check if touch coordinates hit any custom UI buttons on the bottom screen
            // e.g., if (touch.px >= x && touch.px <= x + w ...)
        }

        // Render Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // --- Render Top Screen (Gameplay / World) ---
        C2D_TargetClear(topScreen, C2D_Color32(30, 30, 30, 255));
        C2D_SceneBegin(topScreen);
        // TODO: Render map coordinates (g_mapObjects) here

        // --- Render Bottom Screen (Touch Menu / UI) ---
        C2D_TargetClear(bottomScreen, C2D_Color32(50, 50, 50, 255));
        C2D_SceneBegin(bottomScreen);
        // TODO: Render touch buttons and menu items here

        C3D_FrameEnd(0);
    }

    // 4. Clean up subsystems
    C2D_Fini();
    C3D_Fini();
    ndspExit();
    romfsExit();
    gfxExit();

    return 0;
}
