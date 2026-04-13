#include "simCUDA/pbf/cudaPbfLambda.cuh"

#include <cuda_runtime.h>
#include <math.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaSphKernels.cuh"

namespace
{
     __global__ void computeLambdaKernel(
        int n,
        const float* x,
        const float* y,
        const float* z,
        const float* mass,
        const float* density,
        const int* neighborOffsets,
        const int* neighborIds,
        float* lambda,
        float restDensity,
        float epsilon,
        float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float invRestDensity = 1.0f / restDensity;
        const float gradEps = 1e-6f;

        const float rhoi = density[i];
        const float Ci = rhoi * invRestDensity - 1.0f;

        const float xi = x[i];
        const float yi = y[i];
        const float zi = z[i];

        float sumGrad2 = 0.0f;
        float gradCiX = 0.0f;
        float gradCiY = 0.0f;
        float gradCiZ = 0.0f;

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dz = zi - z[j];
            const float r2 = dx * dx + dy * dy + dz * dz;

            if (r2 < gradEps)
                continue;

            const float r = sqrtf(r2);
            const float invR = 1.0f / r;
            const float gradW = CudaSPH::spikyGradCoeff(r, h);

            const float coeff = mass[j] * invRestDensity * gradW;
            const float gx = coeff * dx * invR;
            const float gy = coeff * dy * invR;
            const float gz = coeff * dz * invR;

            sumGrad2 += gx * gx + gy * gy + gz * gz;
            gradCiX -= gx;
            gradCiY -= gy;
            gradCiZ -= gz;
        }

        sumGrad2 += gradCiX * gradCiX + gradCiY * gradCiY + gradCiZ * gradCiZ;
        lambda[i] = -Ci / (sumGrad2 + epsilon);
    }
}

void launchComputeLambda(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float restDensity,
    float epsilon,
    float smoothingRadius)
{
    if (particles.count <= 0)
        return;

    computeLambdaKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
        particles.z,
        particles.mass,
        particles.density,
        neighbors.offsets,
        neighbors.ids,
        particles.lambda,
        restDensity,
        epsilon,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}