#pragma once
#include <string>
#include "BenchmarkResult.h"

class CsvWriter 
{
public:
    // Создаёт файл с заголовком, если не существует; иначе дописывает
    static void append(const std::string& path, const BenchmarkResult& r);
private:
    static bool fileExists(const std::string& path);
    static void writeHeader(std::ofstream& f);
    static void writeRow(std::ofstream& f, const BenchmarkResult& r);
};