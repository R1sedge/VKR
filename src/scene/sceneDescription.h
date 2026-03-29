#pragma once

#include <vector>
#include <string>
#include "scene/particleRegion.h"
#include "common/Config.h"

struct SceneDescription 
{
    std::string name;
    std::vector<ParticleRegion> regions;

    float gravityX = Config::gravityX;
    float gravityY = Config::gravityY;
};