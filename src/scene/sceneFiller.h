#pragma once

#include "scene/SceneDescription.h"
#include "data/particleData.h"

class SceneFiller
{
public:
    static Particles2D fill(const SceneDescription& desc);

private:
    static void fillRect(const ParticleRegion& region, Particles2D& out);
    static void fillCircle(const ParticleRegion& region, Particles2D& out);

    // Вычисляет шаг сетки: берёт spacing из региона или авто из Config.
    static float resolveSpacing(const ParticleRegion& region);
};