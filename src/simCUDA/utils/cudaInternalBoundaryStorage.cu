#include "simCUDA/utils/cudaInternalBoundaryStorage.cuh"
#include "simCUDA/utils/cudaCheck.h"
#include <algorithm>

__constant__ CudaInternalBoundaryPatch c_internalPatches[MAX_INTERNAL_PATCHES];
__constant__ int c_internalPatchCount;

void uploadInternalPatchesToConstantMemory(const CudaInternalBoundaryPatch* patches, int count)
{
    int n = std::min(count, MAX_INTERNAL_PATCHES);
    CUDA_CHECK(cudaMemcpyToSymbol(c_internalPatches, patches, n * sizeof(CudaInternalBoundaryPatch)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_internalPatchCount, &n, sizeof(int)));
}

void clearInternalPatchesConstantMemory()
{
    int zero = 0;
    CUDA_CHECK(cudaMemcpyToSymbol(c_internalPatchCount, &zero, sizeof(int)));
}