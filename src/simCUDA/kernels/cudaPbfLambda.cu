#include "simCUDA/kernels/cudaPbfLambda.cuh"

#include <cuda_runtime.h>
#include <math.h>

#include "simCUDA/cudaCheck.h"

namespace
{
    constexpr int BLOCK_SIZE = 256;

    int gridSize(int n)
    {
        return (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }

    __device__ float spikyGradCoeffKernel(float r, float h)
    {
        if (r <= 0.0f || r > h)
            return 0.0f;

        const float kPi = 3.14159265358979323846f;
        const float h5 = h * h * h * h * h;
        const float k = -5.0f / (kPi * h5);
        const float x = h - r;
        return k * x * x;
    }

     __global__ void computeLambdaKernel(
        int n,
        const float* x,
        const float* y,
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

        float sumGrad2 = 0.0f;
        float gradCiX = 0.0f;
        float gradCiY = 0.0f;

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float r2 = dx * dx + dy * dy;

            if (r2 < gradEps)
                continue;

            const float r = sqrtf(r2);
            const float invR = 1.0f / r;
            const float gradW = spikyGradCoeffKernel(r, h);

            const float coeff = mass[j] * invRestDensity * gradW;
            const float gx = coeff * dx * invR;
            const float gy = coeff * dy * invR;

            sumGrad2 += gx * gx + gy * gy;
            gradCiX -= gx;
            gradCiY -= gy;
        }

        sumGrad2 += gradCiX * gradCiX + gradCiY * gradCiY;
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

    computeLambdaKernel<<<gridSize(particles.count), BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
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