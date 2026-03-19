#pragma once

namespace CudaUtils
{
    inline constexpr int BLOCK_SIZE = 256;

    inline int gridSize(int n)
    {
        return (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }
}
