#include <3ds.h>

#include <nlohmann/json.hpp>
#include "WorldSeedManager.hpp"
#include <functional>
#include <numeric>
#include <fstream>
#include <iostream>

void WorldSeedManager::setSeed(const std::string& seedStr) {
    currentSeed = seedStr;
    armed = true;
    // Native seeding logic implementation for 3DS
}

void WorldSeedManager::disarm() {
    currentSeed = "";
    armed = false;
}

bool WorldSeedManager::isArmed() const {
    return armed;
}

const std::vector<std::string>& WorldSeedManager::getPipeline() const {
    return pipelineSteps;
}

void WorldSeedManager::runPipeline() {
    if (!armed) return;
    for (const auto& step : pipelineSteps) {
        // Execute each native generation handler step by step
    }
}

// THIS is where you explicitly tell the engine where the binary file is located on the 3DS!
bool WorldSeedManager::loadFromBinaryFile(const std::string& filePath) {
    // Open the file in binary mode ("romfs:/mp_worldseed.bin" or "sdmc:/igo/minidayz/world_seed.bin")
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open binary seed file at: " << filePath << std::endl;
        return false;
    }

    // Get the file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read your binary data into memory structures here...
    // (Example: reading packed header bytes, seed flags, or configuration arrays)

    file.close();
    std::cout << "Successfully loaded world seed binary data from: " << filePath << std::endl;
    return true;
}
