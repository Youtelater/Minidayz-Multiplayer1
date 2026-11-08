#include "KitData.hpp"
#include <fstream>
#include <vector>
#include <iostream>

class KitManager {
public:
    static KitManager& getInstance() {
        static KitManager instance;
        return instance;
    }

    bool loadKitBinary(const std::string& filename = "romfs:/mp_kits.bin") {
        // Open file in binary mode
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "[KitManager] Failed to open " << filename << ". Using default kit." << std::endl;
            loadDefaultKit();
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            parseKitData(buffer);
            std::cout << "[KitManager] Successfully loaded " << filename << " (" << size << " bytes)." << std::endl;
            return true;
        }

        loadDefaultKit();
        return false;
    }

    KitLoadout& getHostKit() { return currentKit; }

private:
    KitManager() {
        // Try loading mp_kits.bin on startup, fallback to defaults if it fails
        loadKitBinary("romfs:/mp_kits.bin");
    }

    void loadDefaultKit() {
        currentKit = {
            235856, // firearm
            853097, // body
            286635, // legs
            854659, // melee
            {726543, 416968, 114514, 542341}, // items
            true    // onRespawn
        };
    }

    void parseKitData(const std::vector<char>& buffer) {
        // If mp_kits.bin has a custom binary layout, parse the bytes here.
        // For example, if the first few bytes represent IDs or flags:
        // if (buffer.size() >= sizeof(KitLoadout)) {
        //     std::memcpy(&currentKit, buffer.data(), sizeof(KitLoadout));
        // }
    }

    KitLoadout currentKit;
};
