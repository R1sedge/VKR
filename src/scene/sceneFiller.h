#pragma once

#include "scene/SceneDescription.h"
#include "data/particleData.h"

class SceneFiller
{
public:
    static Particles3D fill(const SceneDescription& desc);

private:
    static void fillRect(const FluidRegion& r,const VesselBoundary* vessel, Particles3D& out);
    static void fillSphere(const FluidRegion& r, const VesselBoundary* vessel, Particles3D& out);

    static void appendParticle(const FluidRegion& region,
                               const VesselBoundary* vessel,
                               float px, float py, float pz,
                               Particles3D& out);

    // Вычисляет шаг сетки: берёт spacing из региона или авто из Config.
    static float resolveSpacing(const FluidRegion& region);
};