#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct StructurePartSpec {
    std::string type;
    int32_t upIndex = -1;
    int32_t downIndex = -1;
};

struct StructureDefinition {
    std::string name;
    std::vector<StructurePartSpec> parts;
    int32_t hp = 1;
    bool localFrame = false;
    bool fire = false;
    bool plant = false;
    bool lootbox = false;
    std::vector<int32_t> sharedVars;
};

struct ActiveStructure {
    std::string id;
    std::string kind;
    float x = 0.0f;
    float y = 0.0f;
    bool alive = true;
};

struct StructureStats {
    uint32_t built = 0;
    uint32_t requested = 0;
    uint32_t added = 0;
    uint32_t removed = 0;
    uint32_t updates = 0;
    uint32_t booms = 0;
    uint32_t refused = 0;
};

class StructureManager {
public:
    StructureManager();

    // Role & configuration
    void setRole(const std::string& role);
    std::string getRole() const;

    // Simulation & updates
    void update(float deltaTime);
    bool registerStructure(const std::string& id, const std::string& kind, float x, float y);
    void removeStructure(const std::string& id);

    // Binary asset loading matching "mp_structures.bin"
    bool loadFromBinaryFile(const std::string& filePath);

    // Diagnostics & Stats
    StructureStats getStats() const;
    void reset();

private:
    std::string currentRole = "client";
    std::unordered_map<std::string, StructureDefinition> definitions;
    std::unordered_map<std::string, ActiveStructure> activeStructures;
    StructureStats stats;

    void initializeDefinitions();
};
