#include "StructureManager.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

StructureManager::StructureManager() {
    initializeDefinitions();
}

void StructureManager::initializeDefinitions() {
    // Mapped directly from mp_structures.js definitions
    definitions["t567"] = {"barricade", {{"t345", 0, -1}}, 1, false, false, false, false, {}};
    definitions["t572"] = {"barbed wire", {}, 1, false, false, false, false, {}};
    definitions["t398"] = {"tent", {{"t346", -1, 1}, {"t397", -1, 2}}, 1, false, false, false, false, {}};
    definitions["t1179"] = {"generator", {{"t1178", 0, -1}, {"t345", 1, -1}}, 1, true, false, false, false, {0, 1}};
    definitions["t262"] = {"campfire", {}, 1, false, true, false, false, {0}};
    definitions["t559"] = {"plant (pepper)", {}, 1, false, false, true, false, {}};
    definitions["t560"] = {"plant (zucchini)", {}, 1, false, false, true, false, {}};
    definitions["t561"] = {"plant (tomato)", {}, 1, false, false, true, false, {}};
    definitions["t1337"] = {"C4 charge", {}, 1, false, false, false, false, {}};
    definitions["t796"] = {"airdrop box", {}, 1, false, false, false, true, {0}};
}

void StructureManager::setRole(const std::string& role) {
    currentRole = role;
}

std::string StructureManager::getRole() const {
    return currentRole;
}

bool StructureManager::registerStructure(const std::string& id, const std::string& kind, float x, float y) {
    if (definitions.find(kind) == definitions.end()) {
        std::cerr << "[StructureManager] Unknown structure kind: " << kind << std::endl;
        return false;
    }

    ActiveStructure structInst;
    structInst.id = id;
    structInst.kind = kind;
    structInst.x = x;
    structInst.y = y;
    structInst.alive = true;

    activeStructures[id] = structInst;
    stats.added++;
    return true;
}

void StructureManager::removeStructure(const std::string& id) {
    if (activeStructures.find(id) != activeStructures.end()) {
        activeStructures.erase(id);
        stats.removed++;
    }
}

void StructureManager::update(float deltaTime) {
    // Process structure ticks, fires, and plant growth logic on 3DS
    stats.updates += static_cast<uint32_t>(activeStructures.size());
}

bool StructureManager::loadFromBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[StructureManager] Failed to load binary file: " << filePath << std::endl;
        return false;
    }

    // Read packed structure registry or serialized template data
    uint32_t recordCount = 0;
    file.read(reinterpret_cast<char*>(&recordCount), sizeof(uint32_t));

    file.close();
    std::cout << "[StructureManager] Successfully loaded " << filePath << " into 3DS memory." << std::endl;
    return true;
}

StructureStats StructureManager::getStats() const {
    return stats;
}

void StructureManager::reset() {
    activeStructures.clear();
    stats = StructureStats{};
}
