#pragma once

#include <vector>
#include <string>
#include "scene/particleRegion.h"
#include "common/Config.h"

struct BoxBounds
{
    float xMin = 0.0f, xMax = 0.0f;
    float yMin = 0.0f, yMax = 0.0f;
    float zMin = 0.0f, zMax = 0.0f;
};

struct SceneDescription
{
    std::string name;
    std::vector<ParticleRegion> regions;
    BoxBounds bounds;

    float gravityX = Config::gravityX;
    float gravityY = Config::gravityY;
    float gravityZ = Config::gravityZ;
};