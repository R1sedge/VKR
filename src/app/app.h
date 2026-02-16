#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>

#include "render/renderer.h"
#include "sim/engine.h"
// #include "gui/guiLayer.h"
// #include "input/inputManager.h"


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
    //Simulation2D   m_sim;
    //GuiLayer       m_gui;
    //InputManager   m_input;

    bool m_runnig = false;
    bool m_paused = false;
    bool m_stepOnce = false;

    double m_accumulator = 0.0;
    double m_previousTime = 0.0;
};