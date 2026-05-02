#pragma once

#include <vector>

#include "cudaBoundaryPlane.cuh"
#include "scene/boundary/BoundaryPlane.h"

// Загрузить перегородки в __constant__ память GPU
void uploadInternalPatchesToConstantMemory(
    const CudaInternalBoundaryPatch* hostPatches,
    int count);

// Обнулить счётчик перегородок, для смены сцены
void clearInternalPatchesConstantMemory();

// Cконвертировать host-структуры в CUDA-структуры
CudaInternalBoundaryPatch toCuda(const InternalBoundaryPatch& p);