// simCUDA/cudaMemUtils.cuh
#pragma once
#include <cuda_runtime.h>
#include "simCUDA/utils/cudaCheck.h"

namespace CudaMem
{
    inline void allocIntArray(int*& ptr, int count)
    {
        CUDA_CHECK(cudaMalloc(&ptr, sizeof(int) * count));
    }

    inline void freeIntArray(int*& ptr)
    {
        if (ptr) { CUDA_CHECK(cudaFree(ptr)); ptr = nullptr; }
    }

    inline void allocFloatArray(float*& ptr, int count)
    {
        CUDA_CHECK(cudaMalloc(&ptr, sizeof(float) * count));
    }

    inline void freeFloatArray(float*& ptr)
    {
        if (ptr) { CUDA_CHECK(cudaFree(ptr)); ptr = nullptr; }
    }
}
