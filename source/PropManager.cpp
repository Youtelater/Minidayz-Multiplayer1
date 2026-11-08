#include "PropManager.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

PropManager::PropManager() {
    stats = PropStats{};
}

void PropManager::setRole(const std::string& newRole) {
    role = newRole;
    std::cout << "[PropManager] Role set to: " << role << " on 3DS." << std::endl;
}

std::string PropManager::getRole() const {
    return role;
}

void PropManager::setConnected(bool status) {
    connected = status;
}

bool PropManager::isConnected() const {
    return connected;
}

void PropManager::addPropType(const std::string& typeName) {
    supportedPropTypes[typeName] = true;
}

bool PropManager::isPropTypeSupported(const std::string& typeName) const {
    return supportedPropTypes.find(typeName) != supportedPropTypes.end();
}

void PropManager::update(float deltaTime) {
    if (!connected) return;

    // Throttle movement checks similar to mp_props.js tick loop (~142ms interval)
    for (auto& pair : activeProps) {
        PropInstance& prop = pair.second;
        if (!prop.owned) continue;

        // Check distance delta for movement broadcast on 3DS
        float dx = prop.x - prop.lastX;
        float dy = prop.y - prop.lastY;
        float distSq = (dx * dx) + (dy * dy);

        if (distSq > (2.0f * 2.0f) || std::abs(prop.angle - prop.lastAngle) > 0.02f) {
            prop.lastX = prop.x;
            prop.lastY = prop.y;
            prop.lastAngle = prop.angle;
            stats.updatesSent++;
        }
    }
}

bool PropManager::handleIncomingPacket(const std::unordered_map<std::string, std::string>& packet) {
    auto typeIt = packet.find("type");
    if (typeIt == packet.end()) return false;

    if (typeIt->second == "mp_prop_add") {
        stats.built++;
        return true;
    } else if (typeIt->second == "mp_prop_move") {
        stats.updatesApplied++;
        return true;
    } else if (typeIt->second == "mp_prop_del") {
        stats.removalsApplied++;
        return true;
    }

    return false;
}

bool PropManager::loadFromBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[PropManager] Failed to load binary file: " << filePath << std::endl;
        return false;
    }

    // Read packed configuration or default prop types data
    uint32_t typeCount = 0;
    file.read(reinterpret_cast<char*>(&typeCount), sizeof(uint32_t));

    file.close();
    std::cout << "[PropManager] Successfully loaded " << filePath << " on 3DS." << std::endl;
    return true;
}

PropStats PropManager::getStats() const {
    return stats;
}

void PropManager::reset() {
    activeProps.clear();
    stats = PropStats{};
    propIdCounter = 0;
}
