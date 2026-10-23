#include "world.h"
#include <fstream>
#include <nlohmann/json.hpp>

static std::vector<MapObject> g_mapObjects;

void World_LoadData() {
    std::ifstream file("romfs:/data/extracted_audio_and_coords.json");
    if (!file.is_open()) return;

    nlohmann::json j;
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
}

const std::vector<MapObject>& World_GetObjects() {
    return g_mapObjects;
}
