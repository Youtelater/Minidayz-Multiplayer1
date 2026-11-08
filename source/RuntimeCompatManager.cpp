#include <nlohmann/json.hpp>
#include "RuntimeCompatManager.hpp"
#include <fstream>
#include <iostream>

RuntimeCompatManager::RuntimeCompatManager() {
    initializeDefaultMappings();
}

void RuntimeCompatManager::initializeDefaultMappings() {
    // Core runtime aliases extracted from mp_runtime_compat.js
    runtimeAliases["types_by_index"] = "S";
    runtimeAliases["running_layout"] = "wa";
    runtimeAliases["all_global_vars"] = "tD";
    runtimeAliases["objectsByUid"] = "jg";
    runtimeAliases["groups_by_name"] = "wj";
    runtimeAliases["createInstanceFromInit"] = "Sg";
    runtimeAliases["DestroyInstance"] = "$e";

    // Core instance aliases
    instanceAliases["layer"] = "C";
    instanceAliases["angle"] = "u";
    instanceAliases["instance_vars"] = "cc";
    instanceAliases["cur_frame"] = "Y";
    instanceAliases["cur_animation"] = "ab";

    // Sample type name mappings (t1033 -> t1030, etc.)
    typeNameMappings["t1033"] = "t1030";
    typeNameMappings["t1034"] = "t1031";
}

std::string RuntimeCompatManager::getRuntimeAlias(const std::string& key) const {
    auto it = runtimeAliases.find(key);
    return (it != runtimeAliases.end()) ? it->second : key;
}

std::string RuntimeCompatManager::getInstanceAlias(const std::string& key) const {
    auto it = instanceAliases.find(key);
    return (it != instanceAliases.end()) ? it->second : key;
}

std::string RuntimeCompatManager::getTypeNameMapping(const std::string& typeName) const {
    auto it = typeNameMappings.find(typeName);
    return (it != typeNameMappings.end()) ? it->second : typeName;
}

bool RuntimeCompatManager::loadFromBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[RuntimeCompatManager] Failed to load binary file: " << filePath << std::endl;
        return false;
    }

    // Read packed string pair counts and entries sequentially for 3DS optimization
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t keyLen = 0, valLen = 0;
        file.read(reinterpret_cast<char*>(&keyLen), sizeof(uint32_t));
        std::string key(keyLen, '\0');
        file.read(&key[0], keyLen);

        file.read(reinterpret_cast<char*>(&valLen), sizeof(uint32_t));
        std::string val(valLen, '\0');
        file.read(&val[0], valLen);

        runtimeAliases[key] = val;
    }

    file.close();
    std::cout << "[RuntimeCompatManager] Successfully loaded " << filePath << " on 3DS." << std::endl;
    return true;
}

size_t RuntimeCompatManager::getAliasCount() const {
    return runtimeAliases.size() + instanceAliases.size() + typeNameMappings.size();
}
