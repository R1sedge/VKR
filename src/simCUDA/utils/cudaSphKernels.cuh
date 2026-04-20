#pragma once

namespace CudaSPH
{
    inline constexpr float kPi = 3.14159265358979323846f;

    // 3D Poly6: k = 315 / (64 * π * h⁹)
    __host__ __device__ __forceinline__ float poly6(float r, float h)
    {
        if (r < 0.0f || r > h) return 0.0f;
        const float h2 = h * h;
        const float h9 = h2 * h2 * h2 * h2 * h;
        const float k  = 315.0f / (64.0f * kPi * h9);
        const float x  = h2 - r * r;
        return k * x * x * x;
    }

    // 3D Spiky gradient: k = -45 / (π * h⁶)
    __device__ __forceinline__ float spikyGradCoeff(float r, float h)
    {
        if (r <= 0.0f || r > h) return 0.0f;
        const float h6 = h * h * h * h * h * h;
        const float k  = -45.0f / (kPi * h6);
        const float x  = h - r;
        return k * x * x;
    }
}
