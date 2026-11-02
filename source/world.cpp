#include "world.h"
#include <nlohmann/json.hpp>

// Bring in the global merged JSON dataset from main.cpp
extern nlohmann::json g_mainGameData;

static std::vector<MapObject> g_mapObjects;

void World_LoadData() {
    // Read directly from the pre-loaded global dataset instead of opening a file
    if (g_mainGameData.contains("sample_coordinates") && g_mainGameData["sample_coordinates"].is_array()) {
        for (const auto& coord : g_mainGameData["sample_coordinates"]) {
            if (coord.is_array() && coord.size() >= 2) {
                MapObject obj;
                obj.x = coord[0].get<float>();
                obj.y = coord[1].get<float>();
                g_mapObjects.push_back(obj);
            }
        }
    }
}

const std::vector<MapObject>& World_GetObjects() {
    return g_mapObjects;
}
