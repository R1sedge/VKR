#include "CsvWriter.h"
#include <fstream>
#include <filesystem>

bool CsvWriter::fileExists(const std::string& path) 
{
    return std::filesystem::exists(path);
}

void CsvWriter::writeHeader(std::ofstream& f) 
{
    f << "test_name,backend,scene,actual_particles,iterations,repeat_id,"
         "avg_step_ms,median_step_ms,p95_step_ms,std_step_ms,physics_fps,"
         "avg_predict_ms,avg_neighbor_ms,avg_solver_ms,avg_velocity_correct_ms\n";
}

void CsvWriter::writeRow(std::ofstream& f, const BenchmarkResult& r) 
{
    f << r.testName << ','
      << r.backend << ','
      << r.sceneName << ','
      << r.actualParticles << ','
      << r.iterations << ','
      << r.repeatId << ','
      << r.avgStepMs << ','
      << r.medianStepMs << ','
      << r.p95StepMs << ','
      << r.stdStepMs << ','
      << r.physicsFps << ','
      << r.avgPredictMs << ','
      << r.avgNeighborMs << ','
      << r.avgSolverMs << ','
      << r.avgVelocityCorrectMs << '\n';
}

void CsvWriter::append(const std::string& path, const BenchmarkResult& r) 
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());

    bool addHeader = !fileExists(path);
    std::ofstream f(path, std::ios::app);
    if (addHeader) writeHeader(f);
    writeRow(f, r);
}