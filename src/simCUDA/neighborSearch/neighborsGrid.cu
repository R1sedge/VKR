#include "simCUDA/neighborSearch/neighborsGrid.cuh"

#include <cub/cub.cuh>
#include <cuda_runtime.h>
#include <vector>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaMemUtils.cuh"

namespace 
{
    // Kernel 1: каждой частице ставим в соответствие ячейку
    __global__ void assignCellsKernel(
        int n, 
        const float* __restrict__ x, 
        const float* __restrict__ y,
        float left, float bottom, 
        float cs, 
        int cX, int cY,
        int* cellIds, 
        int* particleIds)
    {
        int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        int cx = __float2int_rd((x[i] - left)  / cs);
        int cy = __float2int_rd((y[i] - bottom) / cs);

        cx = max(0, min(cx, cX - 1));
        cy = max(0, min(cy, cY - 1));

        cellIds[i] = cy * cX + cx;
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
        
        if (i == 0  || sortedCells[i-1] != cell) 
            cellStart[cell] = i;

        if (i == n-1 || sortedCells[i+1] != cell) 
            cellEnd[cell] = i+1;
    }

    // Kernel 3: подсчёт соседей (проход 1)
    __global__ void countNeighborsKernel(
        int n, 
        const float* __restrict__ x, 
        const float* __restrict__ y,
        float h2, 
        float left, float bottom, 
        float cs, 
        int cX, int cY,
        const int* __restrict__ sortedIds,
        const int* __restrict__ cellStart,
        const int* __restrict__ cellEnd,
        int* counts)
    {
        int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float xi = x[i];
        float yi = y[i];

        int cx = max(0, min((int)floorf((xi - left)  / cs), cX - 1));
        int cy = max(0, min((int)floorf((yi - bottom) / cs), cY - 1));

        int cnt = 0;

        for (int ny = max(0, cy - 1); ny <= min(cY - 1,cy + 1); ny++)
        {
            for (int nx = max(0, cx - 1); nx <= min(cX - 1, cx + 1); nx++)
            {
                int cell  = ny * cX + nx;
                int begin = cellStart[cell];
                if (begin < 0) continue;
                int end = cellEnd[cell];

                for(int k = begin; k < end; k++)
                {
                    int j = sortedIds[k]; 

                    if (j == i) continue;

                    float dx = xi - x[j];
                    float dy = yi - y[j];

                    if (dx * dx + dy * dy < h2) 
                        cnt++;
                }
            }
        }
        counts[i] = cnt;
    }
    
    // Kernel 4: заполнение соседей (проход 2) 
    __global__ void fillNeighborsKernel(
        int n, 
        const float* __restrict__ x, 
        const float* __restrict__ y,
        float h2, 
        float left, float bottom, 
        float cs, 
        int cX, int cY,
        const int* __restrict__ sortedIds,
        const int* __restrict__ cellStart,
        const int* __restrict__ cellEnd,
        const int* __restrict__ offsets,
        int* ids)
    {
        int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float xi = x[i];
        float yi = y[i];

        int cx = max(0, min((int)floorf((xi - left)  / cs), cX - 1));
        int cy = max(0, min((int)floorf((yi - bottom) / cs), cY - 1));

        int write = offsets[i];

        for(int ny = max(0, cy - 1); ny <= min(cY - 1, cy + 1); ny++)
        {
            for(int nx = max(0, cx - 1); nx <= min(cX - 1,cx + 1); nx++)
            {
                int cell  = ny * cX + nx;
                int begin = cellStart[cell];
                if (begin < 0) continue;
                int end = cellEnd[cell];

                for(int k = begin; k < end; k++)
                {
                    int j = sortedIds[k];

                    if(j == i) continue;

                    float dx = xi - x[j];
                    float dy = yi - y[j];

                    if(dx * dx + dy * dy < h2) 
                        ids[write++] = j;
                }
            }
        }
    }
}

void allocateDeviceUniformGrid(DeviceUniformGrid& g, int n, int totalCells)
{
    freeDeviceUniformGrid(g);

    g.particleCapacity = n;
    g.totalCells = totalCells;

    if(n > 0)
    {
        CudaMem::allocIntArray(g.particleCell, n);
        CudaMem::allocIntArray(g.sortedIds, n);
        CudaMem::allocIntArray(g.keysAlt, n);
        CudaMem::allocIntArray(g.valsAlt, n);
    }

    if(totalCells > 0)
    {
        CudaMem::allocIntArray(g.cellStart, totalCells);
        CudaMem::allocIntArray(g.cellEnd, totalCells);
    }

    // Запрос размера temp-буфера CUB
    if(n > 0)
    {
        cub::DeviceRadixSort::SortPairs(
            nullptr, g.cubTempBytes,
            g.particleCell, g.keysAlt,
            g.sortedIds,    g.valsAlt, n);

        CUDA_CHECK(cudaMalloc(&g.cubTemp, g.cubTempBytes));
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

    if (g.cubTemp)
    { 
        cudaFree(g.cubTemp); 
        g.cubTemp=nullptr; 
    }

    g.particleCapacity = 0;
    g.totalCells = 0;
}

// Главная функция 
void buildNeighborsGridCUDA(
    const DeviceParticles2D& particles,
    DeviceNeighborList& nl,
    DeviceUniformGrid& grid,
    float h,
    float left, 
    float right, 
    float bottom, 
    float top)
{
    const int n = particles.count;
    if (n == 0)
    { 
        nl.idsCount = 0; 
        return; 
    }

    // Пересчёт параметров сетки 
    float cs  = h; // ячейка = 1h
    int cX  = (int)floorf((right - left) / cs) + 1;
    int cY  = (int)floorf((top - bottom) / cs) + 1;
    int tot = cX * cY;

    if (grid.particleCapacity != n || grid.totalCells != tot)
        allocateDeviceUniformGrid(grid, n, tot);

    grid.cellsX = cX; 
    grid.cellsY = cY; 
    grid.cellSize = cs;
    grid.left = left;
    grid.bottom = bottom;

    // 1. Назначение ячеек 
    assignCellsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n, particles.x, particles.y,
        left, bottom, cs, cX, cY,
        grid.particleCell, grid.sortedIds);

    CUDA_CHECK(cudaGetLastError());

    // 2. Radix Sort по cell id
    cub::DeviceRadixSort::SortPairs(
        grid.cubTemp, grid.cubTempBytes,
        grid.particleCell, grid.keysAlt,   // keys  in → out
        grid.sortedIds,    grid.valsAlt,   // values in → out
        n);
    CUDA_CHECK(cudaGetLastError());

    // keysAlt = отсортированные cell id;
    // valsAlt = отсортированные particle id
    std::swap(grid.particleCell, grid.keysAlt);
    std::swap(grid.sortedIds,    grid.valsAlt);

    // 3. Границы ячеек 
    CUDA_CHECK(cudaMemset(grid.cellStart, 0xFF, sizeof(int) * tot)); // -1
    CUDA_CHECK(cudaMemset(grid.cellEnd,   0,    sizeof(int) * tot));

    findCellBoundsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(n, grid.particleCell, grid.cellStart, grid.cellEnd);

    CUDA_CHECK(cudaGetLastError());

    // 4. Аллокация списка соседей
    if (nl.particleCount != n) 
        allocateDeviceNeighborList(nl, n);
    
    const float h2 = h * h;

    // 5. Подсчёт соседей
    countNeighborsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n,
        particles.x, particles.y,
        h2, 
        left, bottom, 
        cs, 
        cX, cY,
        grid.sortedIds, 
        grid.cellStart, 
        grid.cellEnd,
        nl.counts);

    CUDA_CHECK(cudaGetLastError());

    //  6. Prefix sum через CUB (без синхронизации CPU) 
    size_t scanTmp = 0;
    cub::DeviceScan::ExclusiveSum(nullptr, scanTmp, nl.counts, nl.offsets, n + 1);
    void* dScanBuf; 

    CUDA_CHECK(cudaMalloc(&dScanBuf, scanTmp));

    cub::DeviceScan::ExclusiveSum(dScanBuf, scanTmp, nl.counts, nl.offsets, n + 1);

    cudaFree(dScanBuf);
    CUDA_CHECK(cudaGetLastError());
    
    // totalIds = offsets[n] — читаем одно int с GPU
    int totalIds = 0;
    CUDA_CHECK(cudaMemcpy(&totalIds, nl.offsets + n, sizeof(int), cudaMemcpyDeviceToHost));

    // 7. Рост буфера ids при необходимости
    if (totalIds > nl.idsCapacity)
    {
        CudaMem::freeIntArray(nl.ids);
        nl.idsCapacity = totalIds;

        if (totalIds > 0) 
            CudaMem::allocIntArray(nl.ids, totalIds);
    }
    nl.idsCount = totalIds;
    if (totalIds == 0) return;

    // 8. Заполнение ids 
    fillNeighborsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n, 
        particles.x, 
        particles.y,
        h2,
        left, bottom, 
        cs, 
        cX, cY,
        grid.sortedIds, 
        grid.cellStart, 
        grid.cellEnd,
        nl.offsets, 
        nl.ids);
        
    CUDA_CHECK(cudaGetLastError());
}