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
    float3 point;
    float3 normal;
    float3 u;
    float3 v;

    float halfWidth;
    float halfHeight;
    float thickness;

    int apertureType;   // 0 = None, 1 = Circle
    float2 apertureCenter; // В локальных координатах u/v
    float apertureRadius;
};
