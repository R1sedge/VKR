#pragma once

struct FrameTiming 
{
    double predictMs = 0.0;
    double neighborMs = 0.0;
    double velocityCorrectMs = 0.0;
    double totalStepMs = 0.0;
    double solverMs = 0.0;   // суммарно все PBF-итерации
};