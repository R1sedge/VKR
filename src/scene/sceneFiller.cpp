#include "scene/sceneFiller.h"
#include "common/Config.h"

#include <glm/vec3.hpp>
#include <cmath>
#include <cassert>


bool shouldKeepParticle(const FluidRegion& region,
                        const VesselBoundary* vessel,
                        const glm::vec3& p)
{
    if (!region.filterByBoundary)
        return true;

    if (vessel == nullptr || vessel->bodyPatches.empty())
        return true;

    // Держим центр частицы на расстоянии хотя бы radius от стенки.
    return vessel->contains(p, Config::particleRadius);    
}

Particles3D SceneFiller::fill(const SceneDescription& desc)
{
    Particles3D out;

    const VesselBoundary* vessel = desc.vessel.bodyPatches.empty() ? nullptr : &desc.vessel;

    for (const FluidRegion& region : desc.regions)
    {
        switch (region.shape)
        {
            case RegionShape::Rect:fillRect(region, vessel, out); break;
            case RegionShape::Sphere:fillSphere(region, vessel, out); break;
            default: assert(false && "SceneFiller: неизвестный RegionShape");
        }
    }

    out.count = static_cast<int>(out.x.size());
    return out;
}

float SceneFiller::resolveSpacing(const FluidRegion& region)
{
    return (region.spacing > 0.0f) ? region.spacing : Config::particleRadius * 2.1f;
}

void SceneFiller::appendParticle(const FluidRegion& region,
                                 const VesselBoundary* vessel,
                                 float px, float py, float pz,
                                 Particles3D& out)
{
    const glm::vec3 p(px, py, pz);
    if (!shouldKeepParticle(region, vessel, p))
        return;

    out.x.push_back(px);
    out.y.push_back(py);
    out.z.push_back(pz);

    out.px.push_back(px);
    out.py.push_back(py);
    out.pz.push_back(pz);

    out.vx.push_back(region.vx);
    out.vy.push_back(region.vy);
    out.vz.push_back(region.vz);

    out.mass.push_back(Config::particleMass);

    out.density.push_back(0.0f);
    out.lambda.push_back(0.0f);

    out.dx.push_back(0.0f);
    out.dy.push_back(0.0f);
    out.dz.push_back(0.0f);

    out.phase.push_back(region.phase);
}


void SceneFiller::fillRect(const FluidRegion& r,const VesselBoundary* vessel, Particles3D& out)
{
     const float step = resolveSpacing(r);
    assert(step > 0.0f && "Particle spacing must be positive");

    const float R = Config::particleRadius;

    const float startX = r.cx - r.halfX + R;
    const float endX = r.cx + r.halfX - R;

    const float startY = r.cy - r.halfY + R;
    const float endY = r.cy + r.halfY - R;

    const bool isFlatZ = (r.halfZ <= 0.0f);
    const float startZ = isFlatZ ? r.cz : (r.cz - r.halfZ + R);
    const float endZ = isFlatZ ? r.cz : (r.cz + r.halfZ - R);

    if (endX < startX || endY < startY || endZ < startZ)
        return;

    const int cols = std::max(1, static_cast<int>(std::floor((endX - startX) / step)) + 1);
    const int rows = std::max(1, static_cast<int>(std::floor((endY - startY) / step)) + 1);
    const int deps = isFlatZ
        ? 1
        : std::max(1, static_cast<int>(std::floor((endZ - startZ) / step)) + 1);

    for (int iz = 0; iz < deps; ++iz)
    {
        const float pz = isFlatZ ? r.cz : (startZ + iz * step);

        for (int iy = 0; iy < rows; ++iy)
        {
            const float py = startY + iy * step;

            for (int ix = 0; ix < cols; ++ix)
            {
                const float px = startX + ix * step;
                appendParticle(r, vessel, px, py, pz, out);
            }
        }
    }
}

void SceneFiller::fillSphere(const FluidRegion& r, const VesselBoundary* vessel, Particles3D& out)
{
    const float step = resolveSpacing(r);
    assert(step > 0.0f && "Particle spacing must be positive");

    const float usableRadius = r.radius - Config::particleRadius;

    if (usableRadius <= 0.0f)
        return;

    const int cells = std::max(1, static_cast<int>(std::floor((2.0f * usableRadius) / step)) + 1);
    const float start = -usableRadius;

    for (int iz = 0; iz < cells; ++iz)
    {
        const float lz = start + iz * step;

        for (int iy = 0; iy < cells; ++iy)
        {
            const float ly = start + iy * step;

            for (int ix = 0; ix < cells; ++ix)
            {
                const float lx = start + ix * step;

                if (lx * lx + ly * ly + lz * lz > usableRadius * usableRadius)
                    continue;

                appendParticle(r, vessel,
                               r.cx + lx,
                               r.cy + ly,
                               r.cz + lz,
                               out);
            }
        }
    }
}
