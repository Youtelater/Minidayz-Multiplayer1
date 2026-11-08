#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct PlayerStoreRecord {
    std::string id;
    uint64_t storedAt = 0;
    std::string serializedData; // JSON or raw binary blob of checkpoint/journal state
};

struct PlayerStoreStats {
    uint32_t puts = 0;
    uint32_t writes = 0;
    uint32_t writeErrors = 0;
    uint32_t journalWrites = 0;
    uint32_t journalReplays = 0;
    uint32_t legacyImported = 0;
    uint32_t pruned = 0;
    uint32_t fallbackWrites = 0;
};

class PlayerStoreManager {
public:
    PlayerStoreManager();

    // Core Record Operations
    bool putRecord(const std::string& id, const std::string& data);
    bool removeRecord(const std::string& id);
    PlayerStoreRecord* getRecord(const std::string& id);
    std::vector<std::string> getAllIds() const;

    // Pruning & Capacity Management
    void pruneRecords(size_t maxCapacity = 144);

    // Binary asset / save loading matching "mp_playerstore.bin"
    bool loadFromBinaryFile(const std::string& filePath);
    bool saveToBinaryFile(const std::string& filePath) const;

    // Diagnostics & Stats
    PlayerStoreStats getStats() const;
    void reset();

private:
    std::unordered_map<std::string, PlayerStoreRecord> records;
    std::unordered_map<std::string, bool> pendingWrites;
    std::unordered_map<std::string, bool> pendingDeletes;
    PlayerStoreStats stats;

    uint64_t getCurrentTimestamp() const;
};
