#pragma once
#include "simCUDA/utils/cudaBoundaryPlane.cuh"

extern __constant__ CudaInternalBoundaryPatch c_internalPatches[MAX_INTERNAL_PATCHES];
extern __constant__ int c_internalPatchCount;

void uploadInternalPatchesToConstantMemory(const CudaInternalBoundaryPatch* patches, int count);
void clearInternalPatchesConstantMemory();