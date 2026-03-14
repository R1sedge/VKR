#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>

#include "render/renderer.h"
#include "sim/engine.h"
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

private:
    GLFWwindow*    m_window = nullptr;
    Renderer       m_renderer;
    Simulation2D   m_sim;
    UserInterface  m_gui;
    InputManager   m_input;
    AppState       m_state;

    bool m_runnig = false;

    int frameCount = 0;

    double m_previousTime = 0.0;
};