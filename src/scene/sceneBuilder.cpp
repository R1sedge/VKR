#include "scene/sceneBuilder.h"
#include "scene/boundary/boundaryPlane.h"
#include <cassert>
#include <iostream>

// Фабрика 
SceneBuilder SceneBuilder::create(const std::string& name) 
{
    SceneBuilder b;
    b.mdesc.name = name;
    return b;
}

// last() 
FluidRegion* SceneBuilder::last() 
{
    assert(!mdesc.regions.empty() &&
           "SceneBuilder: modifier called before any addBox/addSphere");
    return &mdesc.regions.back();
}

// ──────────── Этап 1: Сосуд ────────────

SceneBuilder& SceneBuilder::setBoxVessel(float halfX, float halfY, float halfZ)                                         
{
    mdesc.vessel = VesselBoundary::makeBox({halfX, halfY, halfZ});
    return *this;
}

SceneBuilder& SceneBuilder::setConvexPrismVessel(const std::vector<glm::vec2>& polygon, float yMin, float yMax) 
{
    mdesc.vessel = VesselBoundary::makeConvexPrism(polygon, yMin, yMax);
    return *this;
}

SceneBuilder& SceneBuilder::addVesselPatch(const BoundaryPatch& patch) 
{
    mdesc.vessel.bodyPatches.push_back(patch);
    return *this;
}

// ──────────── Этап 2: Регионы частиц ────────────
SceneBuilder& SceneBuilder::addFluidBox(float cx, float cy, float cz, float halfX, float halfY, float halfZ) 
{
    FluidRegion r;
    r.shape = RegionShape::Rect;
    r.cx = cx; r.cy = cy; r.cz = cz;
    r.halfX = halfX; r.halfY = halfY; r.halfZ = halfZ;
    mdesc.regions.push_back(r);
    return *this;
}

SceneBuilder& SceneBuilder::addFluidSphere(float cx, float cy, float cz, float radius) 
{
    FluidRegion r;
    r.shape = RegionShape::Sphere;
    r.cx = cx; r.cy = cy; r.cz = cz;
    r.radius = radius;
    mdesc.regions.push_back(r);
    return *this;
}

SceneBuilder& SceneBuilder::withVelocity(float vx, float vy, float vz) 
{
    if (FluidRegion* p = last()) 
    { 
        p->vx = vx; 
        p->vy = vy; 
        p->vz = vz; 
    }
    return *this;
}

SceneBuilder& SceneBuilder::withSpacing(float spacing) 
{
    assert(spacing >= 0.0f && "SceneBuilder: spacing must be >= 0");
    if (FluidRegion* p = last()) p->spacing = spacing;
    return *this;
}

SceneBuilder& SceneBuilder::withPhase(int phase) 
{
    assert(phase >= 0 && "SceneBuilder: phase must be >= 0");
    if (FluidRegion* p = last()) p->phase = phase;
    return *this;
}

SceneBuilder& SceneBuilder::withFilterByBoundary(bool enabled)
{
    if (FluidRegion* p = last()) p->filterByBoundary = enabled;
    return *this;
}

// ──────────── build ────────────

SceneDescription SceneBuilder::build() 
{
    return std::move(mdesc);
}