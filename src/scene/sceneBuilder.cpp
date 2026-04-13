#include "scene/sceneBuilder.h"

#include <cassert>
#include <iostream>

SceneBuilder SceneBuilder::create(const std::string& name)
{
    SceneBuilder builder;
    builder.m_desc.name = name;
    return builder;
}

SceneBuilder& SceneBuilder::addRect(float cx, float cy, float cz, float halfW, float halfH, float halfD)
{
    ParticleRegion r;
    r.shape = RegionShape::Rect;

    r.cx = cx;
    r.cy = cy;
    r.cz = cz;

    r.halfW = halfW;
    r.halfH = halfH;
    r.halfD = halfD;

    m_desc.regions.push_back(r);
    return *this;
}

SceneBuilder& SceneBuilder::addCircle(float cx, float cy, float cz, float radius)
{
    ParticleRegion r;
    r.shape  = RegionShape::Circle;

    r.cx = cx;
    r.cy = cy;
    r.cz = cz;
    r.radius = radius;

    m_desc.regions.push_back(r);
    return *this;
}

ParticleRegion* SceneBuilder::last()
{
    if (m_desc.regions.empty())
    {
        assert(false && "SceneBuilder: modifier called before any addRect/addCircle");
        std::cerr << "[SceneBuilder] WARNING: modifier called with no regions\n";
        return nullptr;
    }
    return &m_desc.regions.back();
}

// Модификаторы последнего региона

SceneBuilder& SceneBuilder::withVelocity(float vx, float vy, float vz)
{
    if (ParticleRegion* p = last())
    {
        p->vx = vx;
        p->vy = vy;
        p->vz = vz;
    }
    return *this;
}

SceneBuilder& SceneBuilder::withMass(float mass)
{
    if (ParticleRegion* p = last())
        p->mass = mass;
    return *this;
}

SceneBuilder& SceneBuilder::withSpacing(float spacing)
{
    assert(spacing >= 0.0f && "SceneBuilder: spacing must be >= 0 (0 = auto)");

    if (ParticleRegion* p = last())
        p->spacing = spacing;
    return *this;
}

SceneBuilder& SceneBuilder::withPhase(int phase)
{
    assert(phase >= 0 && "SceneBuilder: phase must be >= 0");

    if (ParticleRegion* p = last())
        p->phase = phase;
    return *this;
}

SceneBuilder& SceneBuilder::setGravity(float gx, float gy, float gz)
{
    m_desc.gravityX = gx;
    m_desc.gravityY = gy;
    m_desc.gravityZ = gz;
    return *this;
}

SceneDescription SceneBuilder::build()
{
    return std::move(m_desc);
}
