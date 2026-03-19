#pragma once

#include <cuda_runtime.h>
#include <stdexcept>
#include <string>

inline void cudaCheckImpl(cudaError_t err, const char* expr, const char* file, int line)
{
    if (err == cudaSuccess)
        return;

    throw std::runtime_error(
        std::string("CUDA error: ") +
        cudaGetErrorString(err) +
        " | expr: " + expr +
        " | file: " + file +
        " | line: " + std::to_string(line));
}

#define CUDA_CHECK(expr) cudaCheckImpl((expr), #expr, __FILE__, __LINE__)
