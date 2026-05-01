#include "scene/sceneBuilder.h"

#include <cassert>
#include <iostream>

#include "scene/boundary/boundaryPlane.h"
#include "scene/boundary/vesselWireframe.h"

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

// ──────────── Сосуд ────────────

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

// ──────────── Регионы частиц ────────────
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

// ──────────── Внутренние перегородки ────────────

static InternalBoundaryPatch buildBafflePatch(const glm::vec3& point, const glm::vec3& normal, const glm::vec3& upHint,
    float halfWidth, float halfHeight, float thickness)
{
    InternalBoundaryPatch patch;
    patch.point  = point;
    patch.normal = glm::normalize(normal);

    // Устойчивый ортонормированный базис
    glm::vec3 hint = upHint;
    if (glm::abs(glm::dot(hint, patch.normal)) > 0.999f)
        hint = (glm::abs(patch.normal.x) < 0.9f)
               ? glm::vec3(1.f, 0.f, 0.f)
               : glm::vec3(0.f, 1.f, 0.f);

    patch.u = glm::normalize(glm::cross(hint, patch.normal));
    patch.v = glm::normalize(glm::cross(patch.normal, patch.u));

    patch.halfWidth = halfWidth;
    patch.halfHeight = halfHeight;
    patch.thickness = thickness;
    return patch;
}

SceneBuilder& SceneBuilder::addInternalRectBaffle(const glm::vec3& point, const glm::vec3& normal, const glm::vec3& upHint,
    float halfWidth, float halfHeight, float thickness)
{
    auto patch = buildBafflePatch(point, normal, upHint, halfWidth, halfHeight, thickness);
    patch.apertureType = InternalApertureType::None;
    mdesc.vessel.internalPatches.push_back(patch);

    appendBaffleWireframe(mdesc.vessel.wireframe, patch);

    return *this;
}

SceneBuilder& SceneBuilder::addInternalBaffleWithCircularHole(const glm::vec3& point, const glm::vec3& normal, const glm::vec3& upHint,
    float halfWidth, float halfHeight, float thickness, const glm::vec2& holeCenter, float holeRadius)
{
    auto patch = buildBafflePatch(point, normal, upHint, halfWidth, halfHeight, thickness);
    patch.apertureType = InternalApertureType::Circle;
    patch.apertureCenter = holeCenter;
    patch.apertureRadius = holeRadius;
    mdesc.vessel.internalPatches.push_back(patch);

    appendBaffleWireframe(mdesc.vessel.wireframe, patch);

    return *this;
}

// ──────────── build ────────────

SceneDescription SceneBuilder::build() 
{
    return std::move(mdesc);
}