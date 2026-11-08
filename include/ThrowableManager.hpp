#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct ThrowableDefinition {
    std::string name;
    std::string sprite;
    bool hostOnly = false;
    bool freeFall = false;
    bool neverLands = false;
    bool light = false;
    int32_t lightSize = 0;
    int32_t particles = 0;
    std::string leaveEffect = "";
};

struct FlyingThrowable {
    std::string id;
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    float motion = 0.0f;
    float fuse = 0.0f;
    int32_t frame = 0;
    uint64_t timestamp = 0;
    bool sent = false;
};

struct ThrowableStats {
    uint32_t thrown = 0;
    uint32_t ended = 0;
    uint32_t copies = 0;
    uint32_t copyEnds = 0;
    uint32_t relayed = 0;
    uint32_t lostCopies = 0;
    uint32_t airdropsHosted = 0;
    uint32_t airdropsRefused = 0;
};

class ThrowableManager {
public:
    ThrowableManager();

    // Role and sender configuration
    void setRole(const std::string& role);
    std::string getRole() const;

    // Throwable lifecycle & simulation
    void update(float deltaTime);
    bool spawnThrowable(const std::string& id, const std::string& type, float x, float y, float angle, float motion);
    void handleIncomingPacket(const std::unordered_map<std::string, std::string>& packet);
    
    // Binary asset loading matching "mp_throwables.bin"
    bool loadFromBinaryFile(const std::string& filePath);

    // Diagnostics & Stats
    ThrowableStats getStats() const;
    void reset();

private:
    std::string currentRole = "client";
    std::unordered_map<std::string, ThrowableDefinition> definitions;
    std::vector<FlyingThrowable> flyingThrowables;
    std::unordered_map<string, FlyingThrowable> activeCopies;
    ThrowableStats stats;

    void initializeDefinitions();
};
