#include "simCUDA/neighborSearch/neighborsGrid.cuh"

#include <cub/cub.cuh>
#include <cuda_runtime.h>
#include <vector>
#include <utility>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaMemUtils.cuh"
#include "simCUDA/utils/cudaInternalBoundaryUtils.cuh"

namespace
{
    void ensureTempBuffer(void*& ptr, size_t& capacityBytes, size_t requiredBytes)
    {
        if (requiredBytes == 0)
            return;

        if (ptr != nullptr && capacityBytes >= requiredBytes)
            return;

        if (ptr != nullptr)
        {
            CUDA_CHECK(cudaFree(ptr));
            ptr = nullptr;
            capacityBytes = 0;
        }

        CUDA_CHECK(cudaMalloc(&ptr, requiredBytes));
        capacityBytes = requiredBytes;
    }

    // Kernel 1: каждой частице ставим в соответствие 3D ячейку
    __global__ void assignCellsKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        const float* __restrict__ z,
        float left, float bottom, float front,
        float cs,
        int cX, int cY, int cZ,
        int* cellIds,
        int* particleIds)
    {
        int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        int cx = __float2int_rd((x[i] - left) / cs);
        int cy = __float2int_rd((y[i] - bottom) / cs);
        int cz = __float2int_rd((z[i] - front) / cs);

        cx = max(0, min(cx, cX - 1));
        cy = max(0, min(cy, cY - 1));
        cz = max(0, min(cz, cZ - 1));

        cellIds[i] = cz * cY * cX + cy * cX + cx;
        particleIds[i] = i;
    }

    // Kernel 2: находим границы ячеек в отсортированном массиве
    __global__ void findCellBoundsKernel(
        int n,
        const int* __restrict__ sortedCells,
        int* cellStart,
        int* cellEnd)
    {
        int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        int cell = sortedCells[i];

        if (i == 0 || sortedCells[i - 1] != cell) cellStart[cell] = i;
        if (i == n - 1 || sortedCells[i + 1] != cell) cellEnd[cell] = i + 1;
    }

    // Kernel 3: подсчёт соседей (проход 1) — 3D
    __global__ void countNeighborsKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        const float* __restrict__ z,
        float h2,
        float left, float bottom, float front,
        float cs,
        int cX, int cY, int cZ,
        const int* __restrict__ sortedIds,
        const int* __restrict__ cellStart,
        const int* __restrict__ cellEnd,
        int* counts,
        float particleRadius,
        bool  baffleFilterEnabled)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i];
        const float yi = y[i];
        const float zi = z[i];

        const int cx = max(0, min((int)floorf((xi - left) / cs), cX - 1));
        const int cy = max(0, min((int)floorf((yi - bottom) / cs), cY - 1));
        const int cz = max(0, min((int)floorf((zi - front) / cs), cZ - 1));

        int cnt = 0;

        for (int nz = max(0, cz - 1); nz <= min(cZ - 1, cz + 1); ++nz)
        for (int ny = max(0, cy - 1); ny <= min(cY - 1, cy + 1); ++ny)
        for (int nx = max(0, cx - 1); nx <= min(cX - 1, cx + 1); ++nx)
        {
            const int cell = nz * cY * cX + ny * cX + nx;
            const int begin = cellStart[cell];
            if (begin < 0) continue;
            const int end = cellEnd[cell];

            for (int k = begin; k < end; ++k)
            {
                const int j = sortedIds[k];
                if (j == i) continue;

                const float dx = xi - x[j];
                const float dy = yi - y[j];
                const float dz = zi - z[j];
                if (dx * dx + dy * dy + dz * dz >= h2) continue;

                if (baffleFilterEnabled && c_internalPatchCount > 0)
                {
                    const float3 a = make_float3(xi, yi, zi);
                    const float3 b = make_float3(x[j], y[j], z[j]);
                    if (segmentBlockedByAnyInternalPatch(a, b, particleRadius))
                        continue;
                }

                ++cnt;
            }
        }
        counts[i] = cnt;
    }

    // Kernel 4: заполнение соседей (проход 2) — 3D
    __global__ void fillNeighborsKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        const float* __restrict__ z,
        float h2,
        float left, float bottom, float front,
        float cs,
        int cX, int cY, int cZ,
        const int* __restrict__ sortedIds,
        const int* __restrict__ cellStart,
        const int* __restrict__ cellEnd,
        const int* __restrict__ offsets,
        int* ids,
        float particleRadius,
        bool  baffleFilterEnabled)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i];
        const float yi = y[i];
        const float zi = z[i];

        const int cx = max(0, min((int)floorf((xi - left) / cs), cX - 1));
        const int cy = max(0, min((int)floorf((yi - bottom) / cs), cY - 1));
        const int cz = max(0, min((int)floorf((zi - front) / cs), cZ - 1));

        int write = offsets[i];

        for (int nz = max(0, cz - 1); nz <= min(cZ - 1, cz + 1); ++nz)
        for (int ny = max(0, cy - 1); ny <= min(cY - 1, cy + 1); ++ny)
        for (int nx = max(0, cx - 1); nx <= min(cX - 1, cx + 1); ++nx)
        {
            const int cell  = nz * cY * cX + ny * cX + nx;
            const int begin = cellStart[cell];
            if (begin < 0) continue;
            const int end = cellEnd[cell];

            for (int k = begin; k < end; ++k)
            {
                const int j = sortedIds[k];
                if (j == i) continue;

                const float dx = xi - x[j];
                const float dy = yi - y[j];
                const float dz = zi - z[j];
                if (dx * dx + dy * dy + dz * dz >= h2) continue;

                if (baffleFilterEnabled && c_internalPatchCount > 0)
                {
                    const float3 a = make_float3(xi,   yi,   zi);
                    const float3 b = make_float3(x[j], y[j], z[j]);
                    if (segmentBlockedByAnyInternalPatch(a, b, particleRadius))
                        continue;
                }

                ids[write++] = j;
            }
        }
    }

} // namespace


void allocateDeviceUniformGrid(DeviceUniformGrid& g, int n, int totalCells)
{
    freeDeviceUniformGrid(g);

    g.particleCapacity = n;
    g.totalCells = totalCells;

    if (n > 0)
    {
        CudaMem::allocIntArray(g.particleCell, n);
        CudaMem::allocIntArray(g.sortedIds, n);
        CudaMem::allocIntArray(g.keysAlt, n);
        CudaMem::allocIntArray(g.valsAlt, n);
    }

    if (totalCells > 0)
    {
        CudaMem::allocIntArray(g.cellStart, totalCells);
        CudaMem::allocIntArray(g.cellEnd, totalCells);
    }

    if (n > 0)
    {
        size_t requiredSortBytes = 0;
        CUDA_CHECK(cub::DeviceRadixSort::SortPairs(
            nullptr, requiredSortBytes,
            g.particleCell, g.keysAlt,
            g.sortedIds, g.valsAlt, n));

        ensureTempBuffer(g.cubTemp, g.cubTempBytes, requiredSortBytes);

        size_t newScanBytes = 0;
        cub::DeviceScan::ExclusiveSum(
            nullptr, newScanBytes,
            (int*)nullptr, (int*)nullptr, n + 1);

        if (newScanBytes > g.scanTempBytes)
        {
            if (g.scanTemp) { cudaFree(g.scanTemp); g.scanTemp = nullptr; }
            g.scanTempBytes = newScanBytes;
            CUDA_CHECK(cudaMalloc(&g.scanTemp, g.scanTempBytes));
        }
    }
}

void freeDeviceUniformGrid(DeviceUniformGrid& g)
{
    CudaMem::freeIntArray(g.particleCell);
    CudaMem::freeIntArray(g.sortedIds);
    CudaMem::freeIntArray(g.keysAlt);
    CudaMem::freeIntArray(g.valsAlt);
    CudaMem::freeIntArray(g.cellStart);
    CudaMem::freeIntArray(g.cellEnd);

    if (g.cubTemp) { cudaFree(g.cubTemp);  g.cubTemp = nullptr; }
    if (g.scanTemp) { cudaFree(g.scanTemp); g.scanTemp = nullptr; }

    g.cubTempBytes = 0;
    g.scanTempBytes = 0;
    g.particleCapacity = 0;
    g.totalCells = 0;
}


void buildNeighborsGridCUDA(
    const DeviceParticles3D& particles,
    DeviceNeighborList& nl,
    DeviceUniformGrid& grid,
    float h,
    float left, float right,
    float bottom, float top,
    float front, float back,
    float particleRadius,
    bool  baffleFilterEnabled)
{
    const int n = particles.count;
    if (n == 0 || h <= 0.0f)
    {
        nl.idsCount = 0;
        return;
    }

    const float cs = h;
    const int cX  = (int)floorf((right - left) / cs) + 1;
    const int cY  = (int)floorf((top - bottom) / cs) + 1;
    const int cZ  = (int)floorf((back - front) / cs) + 1;
    const int tot = cX * cY * cZ;

    if (grid.particleCapacity != n || grid.totalCells != tot)
        allocateDeviceUniformGrid(grid, n, tot);

    grid.cellsX = cX;
    grid.cellsY = cY;
    grid.cellsZ = cZ;
    grid.cellSize = cs;
    grid.left = left;
    grid.bottom = bottom;
    grid.front = front;
    grid.back = back;

    // 1. Назначение ячеек
    assignCellsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n, particles.x, particles.y, particles.z,
        left, bottom, front, cs, cX, cY, cZ,
        grid.particleCell, grid.sortedIds);
    CUDA_CHECK(cudaGetLastError());

    // 2. Radix Sort по cell id
    cub::DeviceRadixSort::SortPairs(
        grid.cubTemp, grid.cubTempBytes,
        grid.particleCell, grid.keysAlt,
        grid.sortedIds, grid.valsAlt, n);
    CUDA_CHECK(cudaGetLastError());

    std::swap(grid.particleCell, grid.keysAlt);
    std::swap(grid.sortedIds,    grid.valsAlt);

    // 3. Границы ячеек
    CUDA_CHECK(cudaMemset(grid.cellStart, 0xFF, sizeof(int) * tot)); // -1
    CUDA_CHECK(cudaMemset(grid.cellEnd, 0, sizeof(int) * tot));

    findCellBoundsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n, grid.particleCell, grid.cellStart, grid.cellEnd);
    CUDA_CHECK(cudaGetLastError());

    // 4. Аллокация списка соседей
    if (nl.particleCount != n)
        allocateDeviceNeighborList(nl, n);

    const float h2 = h * h;

    // 5. Подсчёт соседей
    countNeighborsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n,
        particles.x, particles.y, particles.z,
        h2,
        left, bottom, front,
        cs, cX, cY, cZ,
        grid.sortedIds, grid.cellStart, grid.cellEnd,
        nl.counts,
        particleRadius,
        baffleFilterEnabled);
    CUDA_CHECK(cudaGetLastError());

    // 6. Prefix sum (ExclusiveSum по n+1 элементам)
    CUDA_CHECK(cudaMemset(nl.counts + n, 0, sizeof(int)));

    size_t requiredScanBytes = 0;
    CUDA_CHECK(cub::DeviceScan::ExclusiveSum(
        nullptr, requiredScanBytes,
        nl.counts, nl.offsets, n + 1));

    ensureTempBuffer(grid.scanTemp, grid.scanTempBytes, requiredScanBytes);

    size_t scanTempBytes = grid.scanTempBytes;
    CUDA_CHECK(cub::DeviceScan::ExclusiveSum(
        grid.scanTemp, scanTempBytes,
        nl.counts, nl.offsets, n + 1));
    CUDA_CHECK(cudaGetLastError());

    // totalIds = offsets[n]
    int totalIds = 0;
    CUDA_CHECK(cudaMemcpy(&totalIds, nl.offsets + n, sizeof(int), cudaMemcpyDeviceToHost));

    // 7. Рост буфера ids при необходимости
    if (totalIds > nl.idsCapacity)
    {
        CudaMem::freeIntArray(nl.ids);
        nl.idsCapacity = (totalIds * 5) / 4; // +25% запас
        CudaMem::allocIntArray(nl.ids, nl.idsCapacity);
    }
    nl.idsCount = totalIds;
    if (totalIds == 0) return;

    // 8. Заполнение ids
    fillNeighborsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n,
        particles.x, particles.y, particles.z,
        h2,
        left, bottom, front,
        cs, cX, cY, cZ,
        grid.sortedIds, grid.cellStart, grid.cellEnd,
        nl.offsets, nl.ids,
        particleRadius,
        baffleFilterEnabled);
    CUDA_CHECK(cudaGetLastError());
}