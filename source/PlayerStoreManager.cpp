#include <nlohmann/json.hpp>
#include "PlayerStoreManager.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <algorithm>

PlayerStoreManager::PlayerStoreManager() {
    stats = PlayerStoreStats{};
}

uint64_t PlayerStoreManager::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

bool PlayerStoreManager::putRecord(const std::string& id, const std::string& data) {
    if (id.empty()) return false;

    PlayerStoreRecord rec;
    rec.id = id;
    rec.storedAt = getCurrentTimestamp();
    rec.serializedData = data;

    records[id] = rec;
    pendingWrites[id] = true;
    pendingDeletes.erase(id);
    stats.puts++;

    pruneRecords();
    return true;
}

bool PlayerStoreManager::removeRecord(const std::string& id) {
    if (records.find(id) == records.end()) return false;

    records.erase(id);
    pendingWrites.erase(id);
    pendingDeletes[id] = true;
    return true;
}

PlayerStoreRecord* PlayerStoreManager::getRecord(const std::string& id) {
    auto it = records.find(id);
    if (it != records.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<std::string> PlayerStoreManager::getAllIds() const {
    std::vector<std::string> ids;
    for (const auto& pair : records) {
        ids.push_back(pair.first);
    }
    return ids;
}

void PlayerStoreManager::pruneRecords(size_t maxCapacity) {
    if (records.size() <= maxCapacity) return;

    std::vector<PlayerStoreRecord> sortedRecs;
    for (const auto& pair : records) {
        sortedRecs.push_back(pair.second);
    }

    std::sort(sortedRecs.begin(), sortedRecs.end(), [](const PlayerStoreRecord& a, const PlayerStoreRecord& b) {
        return a.storedAt > b.storedAt; // Newest first
    });

    while (records.size() > maxCapacity) {
        const auto& oldest = sortedRecs.back();
        records.erase(oldest.id);
        pendingWrites.erase(oldest.id);
        pendingDeletes.erase(oldest.id);
        sortedRecs.pop_back();
        stats.pruned++;
    }
}

bool PlayerStoreManager::loadFromBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[PlayerStoreManager] Failed to load binary file: " << filePath << std::endl;
        return false;
    }

    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t idLen = 0;
        file.read(reinterpret_cast<char*>(&idLen), sizeof(uint32_t));
        std::string id(idLen, '\0');
        file.read(&id[0], idLen);

        uint64_t storedAt = 0;
        file.read(reinterpret_cast<char*>(&storedAt), sizeof(uint64_t));

        uint32_t dataLen = 0;
        file.read(reinterpret_cast<char*>(&dataLen), sizeof(uint32_t));
        std::string data(dataLen, '\0');
        file.read(&data[0], dataLen);

        records[id] = {id, storedAt, data};
    }

    file.close();
    std::cout << "[PlayerStoreManager] Successfully loaded " << records.size() << " records from " << filePath << " on 3DS." << std::endl;
    return true;
}

bool PlayerStoreManager::saveToBinaryFile(const std::string& filePath) const {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[PlayerStoreManager] Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    uint32_t count = static_cast<uint32_t>(records.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

    for (const auto& pair : records) {
        const auto& rec = pair.second;
        uint32_t idLen = static_cast<uint32_t>(rec.id.size());
        file.write(reinterpret_cast<const char*>(&idLen), sizeof(uint32_t));
        file.write(rec.id.data(), idLen);

        file.write(reinterpret_cast<const char*>(&rec.storedAt), sizeof(uint64_t));

        uint32_t dataLen = static_cast<uint32_t>(rec.serializedData.size());
        file.write(reinterpret_cast<const char*>(&dataLen), sizeof(uint32_t));
        file.write(rec.serializedData.data(), dataLen);
    }

    file.close();
    return true;
}

PlayerStoreStats PlayerStoreManager::getStats() const {
    return stats;
}

void PlayerStoreManager::reset() {
    records.clear();
    pendingWrites.clear();
    pendingDeletes.clear();
    stats = PlayerStoreStats{};
}
