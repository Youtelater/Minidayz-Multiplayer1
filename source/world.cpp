#include "world.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdio>

using json = nlohmann::json;

// Define the global vector and grid declared in world.h
std::vector<MapTile> globalLoadedTiles;
int worldGrid[MAP_WIDTH][MAP_HEIGHT];

// Implement the JSON parser for your split map files
void parseMapFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::printf("Failed to open %s\n", filepath.c_str());
        return;
    }

    try {
        json j;
        file >> j;

        // Loop through the JSON array and extract coordinates
        for (auto& item : j) {
            MapTile tile;
            tile.x = item.value("x", 0.0f);
            tile.y = item.value("y", 0.0f);
            tile.tileType = item.value("type", 0);
            
            globalLoadedTiles.push_back(tile);
        }
        std::printf("Successfully parsed %s (%zu tiles loaded)\n", filepath.c_str(), j.size());
    } catch (const json::parse_error& e) {
        std::printf("JSON Parse Error in %s: %s\n", filepath.c_str(), e.what());
    }
}

// Render the parsed tiles using Citro2D
void drawWorldMap(float cameraX, float cameraY) {
    for (const auto& tile : globalLoadedTiles) {
        float screenX = tile.x - cameraX;
        float screenY = tile.y - cameraY;

        // Simple screen culling for the 3DS top screen (400x240)
        if (screenX >= -32.0f && screenX <= 400.0f && screenY >= -32.0f && screenY <= 240.0f) {
            // Draw a placeholder rectangle for each tile
            C2D_DrawRectangle(screenX, screenY, 0.5f, 16.0f, 16.0f, C2D_Color32(120, 180, 100, 255));
        }
    }
}

// Existing asset lifecycle stubs
void initWorldAssets(const char* mapSheetPath) {
    std::printf("World assets initialized with: %s\n", mapSheetPath);
}

void loadBuildingSheet(int index, const char* sheetPath) {
    // Optional building sheet loader implementation
}

void freeWorldAssets() {
    globalLoadedTiles.clear();
}

void generateWorldWithSeed(unsigned int seed) {
    std::printf("World generated with seed: %u\n", seed);
}