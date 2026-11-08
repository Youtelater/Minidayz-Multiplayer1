#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

class WorldStateManager {
public:
    WorldStateManager();

    const std::vector<std::string>& getStateKeys() const;
    void setValue(const std::string& key, float value);
    float getValue(const std::string& key) const;

    std::unordered_map<std::string, float> captureState() const;
    std::string computeStateHash() const;
    void update(float deltaTime);

    // Binary file loader matching "mp_worldstate.bin"
    bool loadFromBinaryFile(const std::string& filePath);

private:
    std::unordered_map<std::string, float> worldStates;
    
    const std::vector<std::string> stateKeys = {
        "Day_survived", "Timer_ALL", "Timer_Hours", "Timer_Hours_Z",
        "Timer_Minutes", "Timer_Minutes_Z", "Day_mode", "Night_light_addition",
        "Day_Night_speed", "Rain", "rains_level", "Rain_speed",
        "Weather_temperature", "Weathercold_current", "Rain_cold",
        "Night_cold", "Day_cold"
    };
};
