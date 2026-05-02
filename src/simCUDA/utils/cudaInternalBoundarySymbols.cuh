#pragma once
#include "cudaBoundaryPlane.cuh"

extern __constant__ CudaInternalBoundaryPatch c_internalPatches[MAX_INTERNAL_PATCHES];
extern __constant__ int c_internalPatchCount;