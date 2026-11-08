#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

class RuntimeCompatManager {
public:
    RuntimeCompatManager();

    // Alias lookup helpers matching mp_runtime_compat mapping
    std::string getRuntimeAlias(const std::string& key) const;
    std::string getInstanceAlias(const std::string& key) const;
    std::string getTypeNameMapping(const std::string& typeName) const;

    // Binary asset loading for 3DS RomFS
    bool loadFromBinaryFile(const std::string& filePath);

    // Diagnostics
    size_t getAliasCount() const;

private:
    std::unordered_map<std::string, std::string> runtimeAliases;
    std::unordered_map<std::string, std::string> instanceAliases;
    std::unordered_map<std::string, std::string> typeNameMappings;

    void initializeDefaultMappings();
};
