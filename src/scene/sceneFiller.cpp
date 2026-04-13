#include "scene/sceneFiller.h"
#include "common/Config.h"
#include <cmath>
#include <cassert>

Particles2D SceneFiller::fill(const SceneDescription& desc)
{
    Particles2D out;

    for (const ParticleRegion& region : desc.regions)
    {
        switch (region.shape)
        {
            case RegionShape::Rect:fillRect(region, out); break;
            case RegionShape::Circle:fillCircle(region, out); break;
            default: assert(false && "SceneFiller: unknown RegionShape");
        }
    }

    out.count = static_cast<int>(out.x.size());
    return out;
}

float SceneFiller::resolveSpacing(const ParticleRegion& region)
{
    return (region.spacing > 0.0f) ? region.spacing : Config::particleRadius * 2.1f;
}

void SceneFiller::fillRect(const ParticleRegion& r, Particles2D& out)
{
    const float step = resolveSpacing(r);

    const int cols = static_cast<int>(2.0f * r.halfW / step) + 1;
    const int rows = static_cast<int>(2.0f * r.halfH / step) + 1;
    const int depths = (r.halfD > 0.0f) ? (static_cast<int>(2.0f * r.halfD / step) + 1) : 1;

    const int n = cols * rows * depths;

    out.x.reserve(out.x.size() + n);
    out.y.reserve(out.y.size() + n);
    out.z.reserve(out.z.size() + n);
    out.px.reserve(out.px.size() + n);
    out.py.reserve(out.py.size() + n);
    out.pz.reserve(out.pz.size() + n);

    out.vx.reserve(out.vx.size() + n);
    out.vy.reserve(out.vy.size() + n);
    out.vz.reserve(out.vz.size() + n);
    out.mass.reserve(out.mass.size() + n);

    out.density.reserve(out.density.size() + n);
    out.lambda.reserve(out.lambda.size() + n);
    out.dx.reserve(out.dx.size() + n);
    out.dy.reserve(out.dy.size() + n);
    out.dz.reserve(out.dz.size() + n);

    out.phase.reserve(out.phase.size() + n);


    for (int depth = 0; depth < depths; ++depth)
    {
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                out.x.push_back(r.cx - r.halfW + col * step);
                out.y.push_back(r.cy - r.halfH + row * step);
                out.z.push_back(r.cz - r.halfD + depth * step);
                out.px.push_back(r.cx - r.halfW + col * step);
                out.py.push_back(r.cy - r.halfH + row * step);
                out.pz.push_back(r.cz - r.halfD + depth * step);

                out.vx.push_back(r.vx);
                out.vy.push_back(r.vy);
                out.vz.push_back(r.vz);
                out.mass.push_back(r.mass);

                out.density.push_back(0.0f);
                out.lambda.push_back(0.0f);
                out.dx.push_back(0.0f);
                out.dy.push_back(0.0f);
                out.dz.push_back(0.0f);

                out.phase.push_back(r.phase);
            }
        }
    }
}

void SceneFiller::fillCircle(const ParticleRegion& r, Particles2D& out)
{
    const float step = resolveSpacing(r);
    const float halfR = Config::particleRadius;

    const int cols = static_cast<int>(2.0f * r.radius / step) + 1;
    const int rows = cols;

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const float px = r.cx - r.radius + col * step;
            const float py = r.cy - r.radius + row * step;

            const float dx = px - r.cx;
            const float dy = py - r.cy;
            if (dx * dx + dy * dy > (r.radius - halfR) * (r.radius - halfR))
                continue;

            out.x.push_back(px);
            out.y.push_back(py);
            out.z.push_back(r.cz);
            out.px.push_back(px);
            out.py.push_back(py);
            out.pz.push_back(r.cz);

            out.vx.push_back(r.vx);
            out.vy.push_back(r.vy);
            out.vz.push_back(r.vz);
            out.mass.push_back(r.mass);

            out.density.push_back(0.0f);
            out.lambda.push_back(0.0f);
            out.dx.push_back(0.0f);
            out.dy.push_back(0.0f);
            out.dz.push_back(0.0f);

            out.phase.push_back(r.phase);
        }
    }
}
