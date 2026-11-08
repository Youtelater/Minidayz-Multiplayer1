#include "NetworkManager.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

NetworkManager::NetworkManager() {
    globalTotals = {0, 0, 0, 0, 0, -1, 0, 0, 0};
}

void NetworkManager::setRole(const std::string& newRole) {
    role = newRole;
}

std::string NetworkManager::getRole() const {
    return role;
}

void NetworkManager::setRoomId(const std::string& id) {
    roomId = id;
}

std::string NetworkManager::getRoomId() const {
    return roomId;
}

void NetworkManager::setSelfId(const std::string& id) {
    selfId = id;
}

std::string NetworkManager::getSelfId() const {
    return selfId;
}

bool NetworkManager::hostRoom(const std::string& id, const std::string& password) {
    roomId = id;
    role = "host";
    active = true;
    std::cout << "[NetworkManager] Hosting room: " << roomId << " on 3DS." << std::endl;
    return true;
}

bool NetworkManager::joinRoom(const std::string& id, const std::string& password) {
    roomId = id;
    role = "client";
    active = true;
    std::cout << "[NetworkManager] Joining room: " << roomId << " on 3DS." << std::endl;
    return true;
}

bool NetworkManager::send(const std::string& peerId, const std::string& data) {
    auto it = peers.find(peerId);
    if (it == peers.end() || !it->second.open) {
        globalTotals.sendFailures++;
        return false;
    }

    uint32_t packetSize = static_cast<uint32_t>(data.size());
    globalTotals.bytesOut += packetSize;
    globalTotals.packetsOut++;

    it->second.stats.bytesOut += packetSize;
    it->second.stats.packetsOut++;
    
    return true;
}

uint32_t NetworkManager::broadcast(const std::string& data, const std::string& exceptPeer) {
    uint32_t successCount = 0;
    for (const auto& pair : peers) {
        if (pair.first == exceptPeer || !pair.second.open) continue;
        if (send(pair.first, data)) {
            successCount++;
        }
    }
    return successCount;
}

bool NetworkManager::isPeerOpen(const std::string& peerId) const {
    auto it = peers.find(peerId);
    return it != peers.end() && it->second.open;
}

std::vector<std::string> NetworkManager::getActivePeers() const {
    std::vector<std::string> activePeers;
    for (const auto& pair : peers) {
        if (pair.second.open) {
            activePeers.push_back(pair.first);
        }
    }
    return activePeers;
}

void NetworkManager::closeConnection(const std::string& peerId) {
    peers.erase(peerId);
}

void NetworkManager::destroy() {
    peers.clear();
    active = false;
    roomId = "";
    selfId = "";
    std::cout << "[NetworkManager] Transport layer destroyed on 3DS." << std::endl;
}

bool NetworkManager::loadFromBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[NetworkManager] Failed to load binary transport config: " << filePath << std::endl;
        return false;
    }

    // Read packed network configurations or presets directly into memory
    file.read(reinterpret_cast<char*>(&globalTotals.bytesIn), sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(&globalTotals.bytesOut), sizeof(uint32_t));
    
    file.close();
    std::cout << "[NetworkManager] Successfully loaded " << filePath << " into 3DS memory." << std::endl;
    return true;
}
