#include <3ds.h>
#include <nlohmann/json.hpp>
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

    // Using DefaultKitData matching your KitData.hpp
    DefaultKitData& getHostKit() { return currentKit; }

private:
    KitManager() {
        loadKitBinary("romfs:/mp_kits.bin");
    }

    void loadDefaultKit() {
        currentKit = {
            235856, // firearm
            853097, // body
            286635, // legs
            854659, // melee
            {726543, 416968, 114514, 542341}, // pocketItems
            true    // onRespawn
        };
    }

    void parseKitData(const std::vector<char>& buffer) {
        // Handle binary parsing here if needed
    }

    DefaultKitData currentKit;
};
