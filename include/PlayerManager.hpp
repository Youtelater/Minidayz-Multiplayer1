#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct InventoryItem {
    std::string netId;
    std::string type;
    std::string slot;
    int oldUid = 0;
    std::vector<int> vars;
};

struct PlayerData {
    uint32_t version = 2;
    uint64_t capturedAt = 0;
    std::string playerId;
    std::string room = "default";
    std::string worldKey;
    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    bool dead = false;
    std::vector<float> vars;
    std::unordered_map<std::string, std::string> loadout;
    std::unordered_map<std::string, std::string> globals;
    std::vector<InventoryItem> items;
};

class PlayerManager {
public:
    PlayerManager();

    // Role & Session Configuration
    void setRole(const std::string& role);
    std::string getRole() const;
    void setRoom(const std::string& roomName);
    void setPlayerId(const std::string& id);

    // Checkpoint & State Handling
    PlayerData captureCheckpoint();
    bool applyCheckpoint(const PlayerData& checkpoint);
    
    // Transactions & Revisions
    uint32_t getLocalRevision() const;
    uint32_t getAckRevision() const;
    void acknowledgeTransaction(uint32_t revision, const std::string& transactionId);

    // Host Peer Management
    void updatePeerPacketTime(const std::string& peerId);
    void removePeer(const std::string& peerId);

    // Tick Loop & Synchronization
    void update(float deltaTime);
    void reset();

private:
    std::string role = "client";
    std::string roomName = "default";
    std::string playerId = "Player_3DS";
    std::string worldKey = "unseeded|map";

    uint32_t localRevision = 1;
    uint32_t ackRevision = 0;
    uint64_t lastTickTime = 0;

    PlayerData currentCheckpoint;
    std::unordered_map<std::string, uint64_t> peerLastPacketTimes;
};
