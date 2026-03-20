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
    void setPhysicsTiming(double seconds);
    void setRenderTiming(double seconds);

    void setSimulationDt(float dt) { simDt = dt; } 
    void setRestDensity(float rd) { currentRestDensity = rd; }
    void setArtPressureEnabled(bool enabled) { currentArtPressureEnabled = enabled; }

private:
    bool initialized = false;

    // Метрики
    float avgFrameMs = 0.0f;
    float avgPhysicsMs = 0.0f;
    float avgRenderMs = 0.0f;

    float avgFps = 0.0f;
    float simDt = 0.0f;

    // Параметры физики
    float currentRestDensity = 200.0f;
    bool currentArtPressureEnabled = true;

    // История
    static constexpr int historySize = 60;

    float frameTimes[historySize] = {0.0f};
    float physicsFrameTimes[historySize] = {0.0f};
    float renderFrameTimes[historySize] = {0.0f};

    int historyIndex = 0;
    int historyCount = 0;
};