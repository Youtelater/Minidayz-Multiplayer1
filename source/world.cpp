#include <nlohmann/json.hpp>
#include "world.h"
#include <msgpack.hpp>
#include <vector>

// Match the exact type defined in main.cpp
extern msgpack::object g_mainGameData;

static std::vector<MapObject> g_mapObjects;

void World_LoadData() {
    g_mapObjects.clear();

    // Check if g_mainGameData is a MAP (dictionary)
    if (g_mainGameData.type == msgpack::type::MAP) {
        for (uint32_t i = 0; i < g_mainGameData.via.map.size; ++i) {
            auto& kv = g_mainGameData.via.map.ptr[i];

            // Safely convert key to std::string
            if (kv.key.type == msgpack::type::STR) {
                std::string key = kv.key.as<std::string>();

                if (key == "sample_coordinates" && kv.val.type == msgpack::type::ARRAY) {
                    for (uint32_t j = 0; j < kv.val.via.array.size; ++j) {
                        auto& coord = kv.val.via.array.ptr[j];

                        if (coord.type == msgpack::type::ARRAY && coord.via.array.size >= 2) {
                            MapObject obj;
                            obj.x = coord.via.array.ptr[0].as<float>();
                            obj.y = coord.via.array.ptr[1].as<float>();
                            g_mapObjects.push_back(obj);
                        }
                    }
                }
            }
        }
    }
}

const std::vector<MapObject>& World_GetObjects() {
    return g_mapObjects;
}
