#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct NetworkStats {
    uint32_t bytesIn = 0;
    uint32_t bytesOut = 0;
    uint32_t packetsIn = 0;
    uint32_t packetsOut = 0;
    uint32_t sendFailures = 0;
    int32_t rttMs = -1;
    uint64_t lastIn = 0;
    uint64_t lastOut = 0;
    uint32_t pingMissed = 0;
};

struct PeerConnection {
    bool open = false;
    uint64_t openedAt = 0;
    NetworkStats stats;
};

class NetworkManager {
public:
    NetworkManager();

    // Core role and room configuration
    void setRole(const std::string& role);
    std::string getRole() const;
    void setRoomId(const std::string& roomId);
    std::string getRoomId() const;
    void setSelfId(const std::string& selfId);
    std::string getSelfId() const;

    // Connection lifecycle
    bool hostRoom(const std::string& roomId, const std::string& password = "");
    bool joinRoom(const std::string& roomId, const std::string& password = "");
    void closeConnection(const std::string& peerId);
    void destroy();

    // Packet transmission & stats tracking
    bool send(const std::string& peerId, const std::string& data);
    uint32_t broadcast(const std::string& data, const std::string& exceptPeer = "");
    
    // Binary asset loading matching "mp_transport.bin"
    bool loadFromBinaryFile(const std::string& filePath);

    // Diagnostics & Stats matching JS export
    NetworkStats getGlobalTotals() const;
    bool isPeerOpen(const std::string& peerId) const;
    std::vector<std::string> getActivePeers() const;

private:
    std::string role = "host";
    std::string roomId = "";
    std::string selfId = "";
    bool active = false;

    // Transport tracking maps
    std::unordered_map<std::string, PeerConnection> peers;
    NetworkStats globalTotals;
    
    // Configuration constants mapped from transport spec
    const std::string HOST_ID = "_xfb8a0f";
};
