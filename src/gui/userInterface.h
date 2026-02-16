#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class UserInterface
{
public:
    UserInterface() = default;
    ~UserInterface();

    bool initialize(GLFWwindow* window);

    void render();

    void setFrameTiming(double frameTimeSeconds);
    void setSimulationDt(float dt) {simDt = dt;}

    bool isPaused() const {return paused;}
    // Возвращает true один раз после нажатия кнопки и сразу сбрасывает флаг
    bool consumeStepOnce() 
    {
        bool v = stepOnce;
        stepOnce = false;
        return v;
    }

private:
    bool initialized = false;

    // Метрики
    float avgFrameMs = 0.0f;
    float avgFps = 0.0f;
    float simDt = 0.0f;

    // История
    static constexpr int historySize = 60;
    float frameTimes[historySize] = {0.0f};
    int histoyIndex = 0;
    int historyCount = 0;

    // Управление
    bool paused = false;
    bool stepOnce = false;
};