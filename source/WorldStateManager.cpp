#include <3ds.h>
#include <nlohmann/json.hpp>
#include "WorldStateManager.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

WorldStateManager::WorldStateManager() {
    // Initialize all state variables to 0.0f
    for (const auto& key : stateKeys) {
        worldStates[key] = 0.0f;
    }
}

const std::vector<std::string>& WorldStateManager::getStateKeys() const {
    return stateKeys;
}

void WorldStateManager::setValue(const std::string& key, float value) {
    if (worldStates.find(key) != worldStates.end()) {
        worldStates[key] = value;
    }
}

float WorldStateManager::getValue(const std::string& key) const {
    auto it = worldStates.find(key);
    if (it != worldStates.end()) {
        return it->second; // Fixed missing return statement
    }
    return 0.0f;
}

std::unordered_map<std::string, float> WorldStateManager::captureState() const {
    return worldStates;
}

std::string WorldStateManager::computeStateHash() const {
    // FNV-1a hash matching the JS _x0c1ff implementation
    uint32_t hash = 2166136261u;
    for (const auto& key : stateKeys) {
        std::string valStr = std::to_string(worldStates.at(key));
        for (char c : valStr) {
            hash ^= static_cast<uint32_t>(c);
            hash *= 17394822u; // Multiplier from JS imul
        }
    }
    
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << (hash >> 0);
    return ss.str();
}

void WorldStateManager::update(float deltaTime) {
    // Native time and weather tick execution for 3DS
    float timerAll = worldStates["Timer_ALL"] + deltaTime;
    worldStates["Timer_ALL"] = timerAll;
    
    // Calculate hours and minutes natively
    int totalMinutes = static_cast<int>(timerAll / 60.0f);
    worldStates["Timer_Hours"] = static_cast<float>(totalMinutes / 60);
    worldStates["Timer_Minutes"] = static_cast<float>(totalMinutes % 60);
}

bool WorldStateManager::loadFromBinaryFile(const std::string& filePath) {
    // Open mp_worldstate.bin in binary mode
    std::ifstream file(filePath, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open binary file at: " << filePath << std::endl;
        return false;
    }

    // Read the packed binary float values sequentially for all 17 state keys
    for (const auto& key : stateKeys) {
        float val = 0.0f;
        if (file.read(reinterpret_cast<char*>(&val), sizeof(float))) {
            worldStates[key] = val;
        } else {
            std::cerr << "Error reading binary data for state key: " << key << std::endl;
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}
