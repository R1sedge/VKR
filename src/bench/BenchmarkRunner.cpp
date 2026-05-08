#include "BenchmarkRunner.h"
#include "sim/simulationBackend.h"
#include <algorithm>
#include <numeric>
#include <cmath>

BenchmarkResult BenchmarkRunner::run(SimulationBackend& backend,
                                     const BenchmarkConfig& cfg,
                                     int repeatId)
{
    backend.setBenchmarkSkipReadback(cfg.skipReadback);

    // Warmup
    for (int i = 0; i < cfg.warmupFrames; ++i)
        backend.update(cfg.dt);

    // Measurement
    std::vector<FrameTiming> timings;
    timings.reserve(cfg.measureFrames);
    for (int i = 0; i < cfg.measureFrames; ++i) {
        backend.update(cfg.dt);
        timings.push_back(backend.getLastFrameTiming());
    }

    backend.setBenchmarkSkipReadback(false);
    return aggregate(timings, cfg, repeatId);
}

BenchmarkResult BenchmarkRunner::aggregate(const std::vector<FrameTiming>& timings, const BenchmarkConfig& cfg, int repeatId)
{
    const int n = static_cast<int>(timings.size());
    BenchmarkResult r;
    r.iterations = cfg.iterations;
    r.repeatId = repeatId;

    // Собираем step_ms в отдельный вектор для статистики
    std::vector<double> steps(n);
    double sumPredict = 0, sumNeighbor = 0, sumSolver = 0, sumVC = 0;
    for (int i = 0; i < n; ++i) 
    {
        steps[i] = timings[i].totalStepMs;
        sumPredict += timings[i].predictMs;
        sumNeighbor += timings[i].neighborMs;
        sumSolver += timings[i].solverMs;
        sumVC += timings[i].velocityCorrectMs;
    }

    // avg
    r.avgStepMs = std::accumulate(steps.begin(), steps.end(), 0.0) / n;
    r.avgPredictMs = sumPredict / n;
    r.avgNeighborMs = sumNeighbor / n;
    r.avgSolverMs = sumSolver / n;
    r.avgVelocityCorrectMs = sumVC / n;

    // median
    std::vector<double> sorted = steps;
    std::sort(sorted.begin(), sorted.end());
    r.medianStepMs = sorted[n / 2];

    // p95
    r.p95StepMs = sorted[static_cast<int>(n * 0.95)];

    // std
    double variance = 0;
    for (double v : steps) variance += (v - r.avgStepMs) * (v - r.avgStepMs);
    r.stdStepMs = std::sqrt(variance / n);

    // fps
    r.physicsFps = r.avgStepMs > 0.0 ? 1000.0 / r.avgStepMs : 0.0;

    return r;
}