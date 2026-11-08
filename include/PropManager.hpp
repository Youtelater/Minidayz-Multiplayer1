#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct PropInstance {
    std::string id;
    std::string type;
    std::string layer;
    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool owned = false;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float lastAngle = 0.0f;
};

struct PropStats {
    uint32_t announced = 0;
    uint32_t built = 0;
    uint32_t updatesSent = 0;
    uint32_t updatesApplied = 0;
    uint32_t removed = 0;
    uint32_t removalsApplied = 0;
    uint32_t unknownType = 0;
    uint32_t noLayer = 0;
};

class PropManager {
public:
    PropManager();

    // Role & configuration
    void setRole(const std::string& role);
    std::string getRole() const;
    void setConnected(bool connected);
    bool isConnected() const;

    // Prop Types Registry
    void addPropType(const std::string& typeName);
    bool isPropTypeSupported(const std::string& typeName) const;

    // Simulation & Synchronization Ticks
    void update(float deltaTime);
    bool handleIncomingPacket(const std::unordered_map<std::string, std::string>& packet);
    
    // Binary asset loading matching "mp_props.bin"
    bool loadFromBinaryFile(const std::string& filePath);

    // Diagnostics & Stats
    PropStats getStats() const;
    void reset();

private:
    std::string role = "client";
    bool connected = false;
    std::unordered_map<std::string, PropInstance> activeProps;
    std::unordered_map<std::string, bool> supportedPropTypes;
    PropStats stats;

    uint64_t lastTickTime = 0;
    uint32_t propIdCounter = 0;
};
