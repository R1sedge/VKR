#pragma once
#include <string>
#include "sim/simulationBackendType.h"

struct BenchmarkConfig 
{
    std::string testName;
    SimulationBackendType backend = SimulationBackendType::CUDA;
    std::string sceneName = "benchmark_box";
    int targetParticles = 10000;
    int actualParticles = 0;    // заполняется после loadScene
    int iterations = 2;
    int warmupFrames = 50;
    int measureFrames = 500;
    int repeats = 3;
    float dt = 1.0f / 90.0f;
    bool skipReadback = true;
    std::string outputPath = "results/raw/output.csv";
};
