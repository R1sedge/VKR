#pragma once
#include "gui/timing/frameTimer.h"

// Полупрозрачный счётчик FPS в правом верхнем углу.
// Цвет зависит от значения: зелёный / жёлтый / красный.
class FpsOverlay
{
public:
    void setParticleCount(int count) { m_particleCount = count; }
    void draw(const FrameTimer& timer);

private:
    int m_particleCount = 0;
};