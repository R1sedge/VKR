#pragma once
#include <GLFW/glfw3.h>

struct AppState;
struct AppCommands;

class UserInterface
{
public:
    UserInterface() = default;
    ~UserInterface();

    bool initialize(GLFWwindow* window);

    void beginFrame();
    void buildUI(const AppState& state, AppCommands& commands);
    void endFrame();

    void setFrameTiming(double frameTimeSeconds);
    void setSimulationDt(float dt) { simDt = dt; } 

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
};