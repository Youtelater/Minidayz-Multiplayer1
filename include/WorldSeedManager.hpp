#pragma once
#include <string>
#include <vector>
#include <cstdint>

class WorldSeedManager {
public:
    void setSeed(const std::string& seedStr);
    void disarm();
    bool isArmed() const;

    const std::vector<std::string>& getPipeline() const;
    void runPipeline();

    // The function that takes the 3DS file path string and loads the .bin file
    bool loadFromBinaryFile(const std::string& filePath);

    int32_t getMapWidth() const { return mapSizeX; }
    int32_t getMapHeight() const { return mapSizeY; }

private:
    std::string currentSeed;
    bool armed = false;
    int32_t mapSizeX = 404400;
    int32_t mapSizeY = 150180;

    const std::vector<std::string> pipelineSteps = {
        "generate_array_locations",
        "generate_array_roads_1",
        "generate_array_roads_2",
        "generate_array_tomap",
        "generate_mapgen_toelements",
        "generate_minimap",
        "generate_helicrashes",
        "generate_bunker_entrance"
    };
};
