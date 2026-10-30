#include <citro2d.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

// 1. Define Structure Types
enum class StructureType {
    TENT,
    CAMPFIRE,
    FENCE,
    GENERATOR
};

// 2. Structure Configuration Data
struct StructureConfig {
    std::string name;
    int maxHp;
    bool producesLight;
};

const std::unordered_map<StructureType, StructureConfig> STRUCTURE_DB = {
    { StructureType::TENT,      { "Tent",      200, false } },
    { StructureType::CAMPFIRE,  { "Campfire",   30, true  } },
    { StructureType::FENCE,     { "Fence",     100, false } },
    { StructureType::GENERATOR, { "Generator", 150, true  } }
};

// 3. Active Placed Structure Instance in the World
struct PlacedStructure {
    uint32_t instanceId;
    StructureType type;
    float x, y;
    int currentHp;
};

class StructureManager {
private:
    std::vector<PlacedStructure> structures;
    uint32_t nextId = 1;
    std::unordered_map<StructureType, C2D_Image> textures;

public:
    // Load textures using citro2d
    void LoadAssets(C2D_SpriteSheet sheet) {
        textures[StructureType::TENT] = C2D_SpriteSheetGetImage(sheet, 0);
        textures[StructureType::CAMPFIRE] = C2D_SpriteSheetGetImage(sheet, 1);
        textures[StructureType::FENCE] = C2D_SpriteSheetGetImage(sheet, 2);
        textures[StructureType::GENERATOR] = C2D_SpriteSheetGetImage(sheet, 3);
    }

    // Spawn a new structure (replaces the JS spawn packet logic)
    uint32_t SpawnStructure(StructureType type, float x, float y) {
        auto it = STRUCTURE_DB.find(type);
        if (it == STRUCTURE_DB.end()) return 0;

        PlacedStructure newStruct;
        newStruct.instanceId = nextId++;
        newStruct.type = type;
        newStruct.x = x;
        newStruct.y = y;
        newStruct.currentHp = it->second.maxHp;

        structures.push_back(newStruct);
        return newStruct.instanceId;
    }

    // Remove a structure by its ID
    void RemoveStructure(uint32_t instanceId) {
        for (auto it = structures.begin(); it != structures.end(); ++it) {
            if (it->instanceId == instanceId) {
                structures.erase(it);
                break;
            }
        }
    }

    // Render all visible structures relative to the player camera
    void Render(float cameraX, float cameraY) {
        for (const auto& s : structures) {
            auto it = textures.find(s.type);
            if (it == textures.end()) continue;

            float screenX = s.x - cameraX;
            float screenY = s.y - cameraY;

            // Draw sprite using citro2d
            C2D_DrawImageAt(it->second, screenX, screenY, 0.5f, NULL, 1.0f, 1.0f);
        }
    }
};
