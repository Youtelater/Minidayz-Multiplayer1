#include "Structure.h"
#include <iostream>

struct PlacedStructure {
    uint32_t instanceId;
    StructureType type;
    float x;
    float y;
    int currentHp;
    int animFrame;
    bool isLit;
    bool isRipe; // For plant types
};

class StructureManager {
private:
    std::vector<PlacedStructure> worldStructures;
    uint32_t nextId = 1;

public:
    // Replicates JS _x61ec: Spawn/register a new structure in the world
    uint32_t SpawnStructure(StructureType type, float x, float y) {
        auto it = STRUCTURE_DATABASE.find(type);
        if (it == STRUCTURE_DATABASE.end()) return 0;

        PlacedStructure newStruct;
        newStruct.instanceId = nextId++;
        newStruct.type = type;
        newStruct.x = x;
        newStruct.y = y;
        newStruct.currentHp = it->second.maxHp;
        newStruct.animFrame = 0;
        newStruct.isLit = false;
        newStruct.isRipe = false;

        worldStructures.push_back(newStruct);
        std::cout << "Spawned " << it->second.name << " at (" << x << ", " << y << ")\n";
        return newStruct.instanceId;
    }

    // Replicates JS _x66e71a: Destroy a structure by ID
    bool RemoveStructure(uint32_t instanceId) {
        for (auto it = worldStructures.begin(); it != worldStructures.end(); ++it) {
            if (it->instanceId == instanceId) {
                worldStructures.erase(it);
                std::cout << "Removed structure ID: " << instanceId << "\n";
                return true;
            }
        }
        return false;
    }

    // Replicates JS _x4a29a: Harvest a ripe plant
    bool HarvestPlant(uint32_t instanceId) {
        for (auto& s : worldStructures) {
            if (s.instanceId == instanceId) {
                if (!STRUCTURE_DATABASE.at(s.type).isPlant) return false;
                if (!s.isRipe) return false;

                s.isRipe = false;
                s.animFrame = 0; // Reset growth stage
                std::cout << "Harvested plant ID: " << instanceId << "\n";
                return true;
            }
        }
        return false;
    }
};
