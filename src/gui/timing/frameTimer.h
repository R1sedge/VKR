#pragma once

// Кольцевой буфер для сглаженных метрик тайминга.
class FrameTimer
{
public:
    // Вызывать один раз за кадр с полным временем кадра (секунды)
    void pushFrame(double frameTimeSeconds);

    // Вызывать после pushFrame (используют тот же historyIndex)
    void pushPhysics(double seconds);
    void pushRender (double seconds);

    float avgFps()       const { return m_avgFps;       }
    float avgFrameMs()   const { return m_avgFrameMs;   }
    float avgPhysicsMs() const { return m_avgPhysicsMs; }
    float avgRenderMs()  const { return m_avgRenderMs;  }

    static constexpr int kSize = 60;

private:

    float m_frame  [kSize] = {};
    float m_physics[kSize] = {};
    float m_render [kSize] = {};

    int   m_index = 0;
    int   m_count = 0;

    float m_avgFps       = 0.0f;
    float m_avgFrameMs   = 0.0f;
    float m_avgPhysicsMs = 0.0f;
    float m_avgRenderMs  = 0.0f;

    // Последний записанный индекс (для physics/render, которые пишут в тот же слот)
    int lastWrittenIndex() const
    {
        return (m_index == 0) ? kSize - 1 : m_index - 1;
    }

    static float average(const float* buf, int count);
};