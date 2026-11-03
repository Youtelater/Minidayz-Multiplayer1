#include "world.h"
#include <msgpack.hpp>
#include <string>
#include <vector>

// Bring in the global binary MessagePack dataset from main.cpp
extern std::string g_mainGameData;

static std::vector<MapObject> g_mapObjects;

void World_LoadData() {
    g_mapObjects.clear();

    if (g_mainGameData.empty()) {
        return;
    }

    try {
        // Unpack the binary MessagePack data from memory
        msgpack::object_handle oh = msgpack::unpack(g_mainGameData.data(), g_mainGameData.size());
        msgpack::object root = oh.get();

        // MessagePack maps require iterating key-value pairs
        if (root.type == msgpack::type::MAP) {
            for (auto& kv : root.via.map) {
                std::string key = kv.key.as<std::string>();
                
                if (key == "sample_coordinates" && kv.val.type == msgpack::type::ARRAY) {
                    for (auto& coord : kv.val.via.array) {
                        if (coord.type == msgpack::type::ARRAY && coord.via.array.size() >= 2) {
                            MapObject obj;
                            obj.x = coord.via.array[0].as<float>();
                            obj.y = coord.via.array[1].as<float>();
                            g_mapObjects.push_back(obj);
                        }
                    }
                }
            }
        }
    } catch (...) {
        // Fallback or catch if unpacking fails
    }
}

const std::vector<MapObject>& World_GetObjects() {
    return g_mapObjects;
}
