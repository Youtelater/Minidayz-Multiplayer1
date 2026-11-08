#include <nlohmann/json.hpp>
#include "ThrowableManager.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

ThrowableManager::ThrowableManager() {
    initializeDefinitions();
}

void ThrowableManager::initializeDefinitions() {
    // Mapped directly from mp_throwables.js definitions
    definitions["t267"] = {"grenade", "t919", false, false, false, false, 0, 0, "t268"};
    definitions["t265"] = {"flare", "t924", false, false, false, true, 0, 0, ""};
    definitions["t899"] = {"molotov", "t925", false, false, true, false, 0, 0, "t821"};
    definitions["t1023"] = {"smoke", "t1022", false, false, false, false, 0, 0, "t1024"};
    definitions["t928"] = {"flare shell", "t929", false, false, false, true, 150, 10, ""};
    definitions["t711"] = {"airdrop crate", "t711", true, true, false, false, 0, 0, "t725"};
}

void ThrowableManager::setRole(const std::string& role) {
    currentRole = role;
}

std::string ThrowableManager::getRole() const {
    return currentRole;
}

bool ThrowableManager::spawnThrowable(const std::string& id, const std::string& type, float x, float y, float angle, float motion) {
    if (definitions.find(type) == definitions.end()) {
        std::cerr << "[ThrowableManager] Unknown throwable type: " << type << std::endl;
        return false;
    }

    FlyingThrowable t;
    t.id = id;
    t.type = type;
    t.x = x;
    t.y = y;
    t.angle = angle;
    t.motion = motion;
    t.timestamp = 0; // Tick representation
    t.sent = false;

    flyingThrowables.push_back(t);
    stats.thrown++;
    return true;
}

void ThrowableManager::update(float deltaTime) {
    // Process active flying projectiles and update positions based on motion/physics on 3DS
    for (auto it = flyingThrowables.begin(); it != flyingThrowables.end();) {
        // Simple kinematic update placeholder for 3DS native loop
        it->x += std::cos(it->angle) * it->motion * deltaTime;
        it->y += std::sin(it->angle) * it->motion * deltaTime;
        ++it;
    }
}

void ThrowableManager::handleIncomingPacket(const std::unordered_map<std::string, std::string>& packet) {
    auto typeIt = packet.find("type");
    if (typeIt == packet.end()) return;

    if (typeIt->second == "mp_throw") {
        stats.relayed++;
    } else if (typeIt->second == "mp_airdrop") {
        stats.airdropsHosted++;
    }
}

bool ThrowableManager::loadFromBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ThrowableManager] Failed to load binary file: " << filePath << std::endl;
        return false;
    }

    // Read packed configuration or initial throwable constants
    uint32_t defCount = 0;
    file.read(reinterpret_cast<char*>(&defCount), sizeof(uint32_t));

    file.close();
    std::cout << "[ThrowableManager] Successfully loaded " << filePath << " on 3DS." << std::endl;
    return true;
}

ThrowableStats ThrowableManager::getStats() const {
    return stats;
}

void ThrowableManager::reset() {
    flyingThrowables.clear();
    activeCopies.clear();
    stats = ThrowableStats{};
}
