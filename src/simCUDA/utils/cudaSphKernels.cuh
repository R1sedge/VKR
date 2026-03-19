#pragma once

namespace CudaSPH
{
    inline constexpr float kPi = 3.14159265358979323846f;

    __host__ __device__ __forceinline__ float poly6(float r, float h)
    {
        if (r < 0.0f || r > h) return 0.0f;
        const float h2 = h * h;
        const float k  = 4.0f / (kPi * h2 * h2 * h2 * h2);
        const float x  = h2 - r * r;
        return k * x * x * x;
    }

    __device__ __forceinline__ float spikyGradCoeff(float r, float h)
    {
        if (r <= 0.0f || r > h) return 0.0f;
        const float h5 = h * h * h * h * h;
        const float k  = -5.0f / (kPi * h5);
        const float x  = h - r;
        return k * x * x;
    }
}
