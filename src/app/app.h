#pragma once
#include <GLFW/glfw3.h>

// foraward declaration
class Renderer;
class Simulation2D;
class GuiLayer;
class InputManager;

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
    GLFWwindow*                     m_window = nullptr;
    std::unique_ptr<Renderer>       m_renderer;
    std::unique_ptr<Simulation2D>   m_sim;
    std::unique_ptr<GuiLayer>       m_gui;
    std::unique_ptr<InputManager>   m_input;

    bool m_runnig = false;
};