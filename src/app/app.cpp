#include "app.h"

#include <stdexcept>
#include <iostream>

#include "common/config.h"

App::App(): 
    m_renderer(Config::windowWidth, Config::windowHeight, nullptr),
    m_gui(),
    m_sim()
    {}

App::~App() = default;

bool App::initialize()
{
    if (!glfwInit())
    {
		std::cerr << "GLFW init failed\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(Config::windowWidth, 
                                Config::windowHeight, 
                                "Simulation", nullptr, nullptr);
	if (!m_window)
	{
        std::cerr << "Window creation failed\n";
		glfwTerminate();
		return false;
	}

    glfwMakeContextCurrent(m_window);
	glfwSwapInterval(1); // v-sync

    if (!gladLoadGL())
	{   
        std::cerr << "GLAD init failed\n";
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
        return false;
    }

    m_renderer.setWindow(m_window);

    m_gui.initialize(m_window);
    m_gui.setSimulationDt(Config::dt);

    float halfWorldW = Config::windowWidth / (2.0f * Config::pixelsPerUnits);
    float halfWorldH = Config::windowHeight / (2.0f * Config::pixelsPerUnits);

    m_sim.setWorldBounds(-halfWorldW, halfWorldW, 
                         -halfWorldH, halfWorldH);

    m_runnig = true;
    return true;
}

 void App::shutDown()
 {
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
    m_runnig=false;
 }

 void App::run()
 {
    if(!m_runnig) return;
    mainLoop();
 }

void App::mainLoop()
{
    const double dt = Config::dt;
    const double maxFrameTime = 0.25;

    m_previousTime = glfwGetTime();
    m_accumulator = 0.0;

    while (m_runnig && !glfwWindowShouldClose(m_window))
    {
        double currentTime = glfwGetTime();
        double frameTime = currentTime - m_previousTime;
        m_previousTime = currentTime;

        if (frameTime > maxFrameTime) 
            frameTime = maxFrameTime;

        m_gui.setFrameTiming(frameTime);

        bool paused = m_gui.isPaused();

        if (!paused)
        {
            m_accumulator += frameTime;

            while (m_accumulator >= dt)
            {
                update(dt);
                m_accumulator -= dt;
            }
        }
        else
        {
            if (m_gui.consumeStepOnce())
            {
                update(dt);
            }
        }

        render();
        m_gui.render();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void App::update(float dt)
{
    m_sim.update(dt);
}

void App::render()
{  
    m_renderer.renderFrame();
}