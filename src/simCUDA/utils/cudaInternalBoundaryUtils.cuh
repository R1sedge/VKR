#pragma once

#include "simCUDA/utils/cudaBoundaryPlane.cuh"
#include "simCUDA/utils/cudaInternalBoundaryStorage.cuh"

__device__ __forceinline__ bool insideAperture(
    float localU, float localV,
    const CudaInternalBoundaryPatch& patch,
    float particleRadius)
{
    if (patch.apertureType != 1) return false;
    const float du = localU - patch.apertureCenterU;
    const float dv = localV - patch.apertureCenterV;
    const float effectiveR = fmaxf(0.0f, patch.apertureRadius - particleRadius);
    return du * du + dv * dv <= effectiveR * effectiveR;
}

__device__ __forceinline__ bool segmentBlockedByInternalPatch(
    float3 a, float3 b,
    const CudaInternalBoundaryPatch& patch,
    float particleRadius)
{
    const float sideA = (a.x - patch.pointX) * patch.normalX
                      + (a.y - patch.pointY) * patch.normalY
                      + (a.z - patch.pointZ) * patch.normalZ;
    const float sideB = (b.x - patch.pointX) * patch.normalX
                      + (b.y - patch.pointY) * patch.normalY
                      + (b.z - patch.pointZ) * patch.normalZ;

    // Оба по одну сторону — пересечения нет
    if (signbit(sideA) == signbit(sideB)) return false;

    const float denom = sideA - sideB;
    if (fabsf(denom) < 1e-6f) return false;

    const float t = sideA / denom;
    const float hx = a.x + t * (b.x - a.x) - patch.pointX;
    const float hy = a.y + t * (b.y - a.y) - patch.pointY;
    const float hz = a.z + t * (b.z - a.z) - patch.pointZ;

    const float localU = hx * patch.uX + hy * patch.uY + hz * patch.uZ;
    const float localV = hx * patch.vX + hy * patch.vY + hz * patch.vZ;

    if (fabsf(localU) > patch.halfWidth  + particleRadius) return false;
    if (fabsf(localV) > patch.halfHeight + particleRadius) return false;

    // Сегмент проходит через отверстие — не заблокирован
    if (insideAperture(localU, localV, patch, particleRadius)) return false;

    return true;
}

__device__ __forceinline__ bool segmentBlockedByAnyInternalPatch(
    float3 a, float3 b, float particleRadius)
{
    for (int k = 0; k < c_internalPatchCount; ++k)
    {
        if (segmentBlockedByInternalPatch(a, b, c_internalPatches[k], particleRadius))
            return true;
    }
    return false;
}