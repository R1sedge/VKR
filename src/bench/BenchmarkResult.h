#pragma once
#include <string>

struct BenchmarkResult 
{
    std::string testName;
    std::string backend;
    std::string sceneName;
    int actualParticles = 0;
    int iterations = 0;
    int repeatId = 0;

    // Полный шаг
    double avgStepMs = 0.0;
    double medianStepMs = 0.0;
    double p95StepMs = 0.0;
    double stdStepMs = 0.0;
    double physicsFps = 0.0;

    // Разбивка по этапам
    double avgPredictMs = 0.0;
    double avgNeighborMs = 0.0;
    double avgSolverMs = 0.0;
    double avgVelocityCorrectMs = 0.0;
};