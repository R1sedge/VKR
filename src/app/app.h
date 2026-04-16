#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>

#include "render/renderer.h"
#include "render/Camera3D.h"  

#include "sim/simulationBackend.h"
#include "gui/userInterface.h"
#include "input/inputManager.h"
#include "app/appState.h"
#include "app/appCommands.h"

class App
{
public:
    App();
    ~App();

    bool initialize();
    void shutDown();
    void run();

private:
    void mainLoop();
    void update(float dt);
    void render();
    void applyCommands(AppCommands& cmd);

    bool initializeGLFW();
    bool createWindow();
    bool initializeGLAD();
    bool setWindow();
    bool initializeIMGUI(int idx);
    bool initializeCamera();
    
    bool initializeScene(int idx);

private:
    GLFWwindow* m_window = nullptr;
    Renderer m_renderer;
    Camera3D m_camera; 

    SimulationBackendType m_backendType = SimulationBackendType::CUDA;
    SimulationBackend m_sim;

    UserInterface m_gui;
    InputManager m_input;
    AppState m_state;

    bool m_running = false;
    int frameCount = 0;
    double m_previousTime = 0.0;
    bool m_interopEnabled = false;
};