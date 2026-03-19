#pragma once

#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

#include <cstddef>

// Равномерная сетка на GPU
struct DeviceUniformGrid 
{
    int   cellsX     = 0;
    int   cellsY     = 0;
    int   totalCells = 0;
    float cellSize   = 0.f;
    float left       = 0.f;
    float bottom     = 0.f;
    int   particleCapacity = 0;

    int*   particleCell = nullptr;  // [n] - ключи (cell id) для сортировки
    int*   sortedIds    = nullptr;  // [n] - значения (particle id) после сортировки
    int*   keysAlt      = nullptr;  // [n] - буфер CUB
    int*   valsAlt      = nullptr;  // [n] - буфер CUB
    int*   cellStart    = nullptr;  // [totalCells]
    int*   cellEnd      = nullptr;  // [totalCells]

    void*  cubTemp      = nullptr;
    size_t cubTempBytes = 0;
};

void allocateDeviceUniformGrid(DeviceUniformGrid& g, int n, int totalCells);
void freeDeviceUniformGrid(DeviceUniformGrid& g);

// Основная функция - полный цикл построения соседей через сетку
void buildNeighborsGridCUDA(
    const DeviceParticles2D& particles,
    DeviceNeighborList& nl,
    DeviceUniformGrid& grid,
    float smoothingRadius,
    float worldLeft, float worldRight,
    float worldBottom, float worldTop
);
