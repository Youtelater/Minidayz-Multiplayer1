#pragma once
#include <vector>

struct MapObject {
    float x;
    float y;
};

void World_LoadData();
const std::vector<MapObject>& World_GetObjects();
