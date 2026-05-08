#pragma once
#include <vector>
#include "BenchmarkConfig.h"
#include "BenchmarkResult.h"
#include "FrameTiming.h"

class SimulationBackend;

class BenchmarkRunner {
public:
    // Запускает warmup + measure для одного повтора.
    // backend уже загружен сценой и сконфигурирован.
    BenchmarkResult run(SimulationBackend& backend, const BenchmarkConfig& cfg, int repeatId);

    static BenchmarkResult aggregate(const std::vector<FrameTiming>& timings, const BenchmarkConfig& cfg, int repeatId);
};