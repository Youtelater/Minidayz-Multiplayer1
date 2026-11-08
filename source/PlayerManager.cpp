#include "PlayerManager.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

PlayerManager::PlayerManager() {
    currentCheckpoint = PlayerData{};
}

void PlayerManager::setRole(const std::string& newRole) {
    role = newRole;
    std::cout << "[PlayerManager] Role set to: " << role << " on 3DS." << std::endl;
}

std::string PlayerManager::getRole() const {
    return role;
}

void PlayerManager::setRoom(const std::string& newRoom) {
    roomName = newRoom;
}

void PlayerManager::setPlayerId(const std::string& id) {
    playerId = id;
}

PlayerData PlayerManager::captureCheckpoint() {
    currentCheckpoint.version = 2;
    currentCheckpoint.playerId = playerId;
    currentCheckpoint.room = roomName;
    currentCheckpoint.worldKey = worldKey;
    currentCheckpoint.capturedAt = static_cast<uint64_t>(std::time(nullptr));
    
    // In a full engine integration, position and inventory are populated from C2 runtime instances here
    return currentCheckpoint;
}

bool PlayerManager::applyCheckpoint(const PlayerData& checkpoint) {
    if (checkpoint.playerId.empty()) {
        std::cerr << "[PlayerManager] Refused empty or invalid checkpoint!" << std::endl;
        return false;
    }

    currentCheckpoint = checkpoint;
    localRevision = std::max(localRevision, checkpoint.version);
    
    std::cout << "[PlayerManager] Successfully applied checkpoint for player: " 
              << checkpoint.playerId << " at (" << checkpoint.x << ", " << checkpoint.y << ")" << std::endl;
    return true;
}

uint32_t PlayerManager::getLocalRevision() const {
    return localRevision;
}

uint32_t PlayerManager::getAckRevision() const {
    return ackRevision;
}

void PlayerManager::acknowledgeTransaction(uint32_t revision, const std::string& transactionId) {
    if (revision >= ackRevision) {
        ackRevision = revision;
        std::cout << "[PlayerManager] Transaction acknowledged up to revision: " << ackRevision << std::endl;
    }
}

void PlayerManager::updatePeerPacketTime(const std::string& peerId) {
    peerLastPacketTimes[peerId] = static_cast<uint64_t>(std::time(nullptr));
}

void PlayerManager::removePeer(const std::string& peerId) {
    peerLastPacketTimes.erase(peerId);
}

void PlayerManager::update(float deltaTime) {
    if (role == "host") {
        uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
        // Check for timed-out peers (~15 second timeout window)
        for (auto it = peerLastPacketTimes.begin(); it != peerLastPacketTimes.end();) {
            if (currentTime - it->second > 15) {
                std::cout << "[PlayerManager] Peer timed out: " << it->first << std::endl;
                it = peerLastPacketTimes.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void PlayerManager::reset() {
    localRevision = 1;
    ackRevision = 0;
    currentCheckpoint = PlayerData{};
    peerLastPacketTimes.clear();
    std::cout << "[PlayerManager] Reset state on 3DS." << std::endl;
}
