#include "frameTimer.h"
#include <cmath>
#include <algorithm>

float FrameTimer::average(const float* buf, int count)
{
    if (count <= 0) return 0.0f;

    float sum = 0.0f;
    for (int i = 0; i < count; ++i) sum += buf[i];

    return sum / static_cast<float>(count);
}

void FrameTimer::pushFrame(double frameTimeSeconds)
{
    const float ms  = static_cast<float>(frameTimeSeconds * 1000.0);
    const float fps = (frameTimeSeconds > 0.0)
                    ? static_cast<float>(1.0 / frameTimeSeconds)
                    : 0.0f;

    m_frame[m_index] = ms;
    m_index = (m_index + 1) % kSize;
    if (m_count < kSize) ++m_count;

    m_avgFrameMs = average(m_frame, m_count);
    m_avgFps     = (m_avgFrameMs > 0.0f) ? (1000.0f / m_avgFrameMs) : fps;
}

void FrameTimer::pushPhysics(double seconds)
{
    m_physics[lastWrittenIndex()] = static_cast<float>(seconds * 1000.0);
    m_avgPhysicsMs = average(m_physics, m_count);
}

void FrameTimer::pushRender(double seconds)
{
    m_render[lastWrittenIndex()] = static_cast<float>(seconds * 1000.0);
    m_avgRenderMs = average(m_render, m_count);
}