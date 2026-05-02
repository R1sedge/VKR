#include "cudaInternalBoundaryStorage.cuh"
#include "cudaCheck.h"
#include <algorithm>

// Symbols в constant memory 
__constant__ CudaInternalBoundaryPatch c_internalPatches[MAX_INTERNAL_PATCHES];
__constant__ int c_internalPatchCount;


void uploadInternalPatchesToConstantMemory(
    const CudaInternalBoundaryPatch* hostPatches,
    int count)
{
    const int clamped = std::min(count, MAX_INTERNAL_PATCHES);

    CUDA_CHECK(cudaMemcpyToSymbol(c_internalPatches, hostPatches, clamped * sizeof(CudaInternalBoundaryPatch),  
        0, cudaMemcpyHostToDevice));

    CUDA_CHECK(cudaMemcpyToSymbol(c_internalPatchCount, &clamped, sizeof(int), 0, cudaMemcpyHostToDevice));
}

void clearInternalPatchesConstantMemory()
{
    const int zero = 0;
    CUDA_CHECK(cudaMemcpyToSymbol(c_internalPatchCount, &zero, sizeof(int), 0, cudaMemcpyHostToDevice));
}

CudaInternalBoundaryPatch toCuda(const InternalBoundaryPatch& p)
{
    CudaInternalBoundaryPatch cp{};

    cp.point = make_float3(p.point.x,  p.point.y,  p.point.z);
    cp.normal = make_float3(p.normal.x, p.normal.y, p.normal.z);
    cp.u = make_float3(p.u.x, p.u.y, p.u.z);
    cp.v = make_float3(p.v.x, p.v.y,p.v.z);

    cp.halfWidth = p.halfWidth;
    cp.halfHeight = p.halfHeight;
    cp.thickness = p.thickness;

    cp.apertureType = static_cast<int>(p.apertureType);
    cp.apertureCenter = make_float2(p.apertureCenter.x, p.apertureCenter.y);
    cp.apertureRadius = p.apertureRadius;

    return cp;
}