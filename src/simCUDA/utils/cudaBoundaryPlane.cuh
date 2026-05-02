#pragma once

#include <vector>

#include "scene/boundary/boundaryPlane.h"

// Плотное device-представление плоскости без зависимостей от GLM.
// Храним point + normal как шесть float, чтобы безопасно копировать
// host-side BoundaryPlane -> device-side массив через cudaMemcpy.
struct DeviceBoundaryPlane
{
    float px = 0.0f;
    float py = 0.0f;
    float pz = 0.0f;

    float nx = 0.0f;
    float ny = 1.0f;
    float nz = 0.0f;
};

inline DeviceBoundaryPlane makeDeviceBoundaryPlane(const BoundaryPlane& plane)
{
    DeviceBoundaryPlane out;
    out.px = plane.point.x;
    out.py = plane.point.y;
    out.pz = plane.point.z;
    out.nx = plane.normal.x;
    out.ny = plane.normal.y;
    out.nz = plane.normal.z;
    return out;
}

inline std::vector<DeviceBoundaryPlane> makeDeviceBoundaryPlanes(const std::vector<BoundaryPlane>& planes)
{
    std::vector<DeviceBoundaryPlane> out;
    out.reserve(planes.size());

    for (const BoundaryPlane& plane : planes)
        out.push_back(makeDeviceBoundaryPlane(plane));

    return out;
}

#define MAX_INTERNAL_PATCHES 5

struct CudaInternalBoundaryPatch
{
    float pointX,  pointY,  pointZ;   // центр
    float normalX, normalY, normalZ;  // нормаль

    float uX, uY, uZ;                 // локальный базис U
    float vX, vY, vZ;                 // локальный базис V

    float halfWidth, halfHeight;
    float thickness;

    int apertureType;               // 0=нет, 1=круг
    float apertureCenterU, apertureCenterV;
    float apertureRadius;
};

// Конвертер host → device
inline CudaInternalBoundaryPatch toCuda(const InternalBoundaryPatch& p)
{
    CudaInternalBoundaryPatch out{};
    out.pointX = p.point.x;  out.pointY = p.point.y; out.pointZ  = p.point.z;
    out.normalX = p.normal.x; out.normalY = p.normal.y; out.normalZ = p.normal.z;

    out.uX = p.u.x; out.uY = p.u.y; out.uZ = p.u.z;
    out.vX = p.v.x; out.vY = p.v.y; out.vZ = p.v.z;

    out.halfWidth = p.halfWidth;
    out.halfHeight = p.halfHeight;
    out.thickness = p.thickness;

    out.apertureType = static_cast<int>(p.apertureType);
    out.apertureCenterU = p.apertureCenter.x;
    out.apertureCenterV = p.apertureCenter.y;
    out.apertureRadius = p.apertureRadius;
    return out;
}
