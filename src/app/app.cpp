#include "app.h"

#include <stdexcept>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "render/renderer.h"
#include "sim/engine.h"
#include "common/config.h"

App::App() = default;

App::~App()
{
    shutDown();
}

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

    m_renderer = std::make_unique<Renderer>(Config::windowWidth, Config::windowHeight, m_window);
    m_sim = std::make_unique<Simulation2D>();
    //m_gui =  std::make_unique<Guilayer>(m_window);
    //m_input =  std::make_unique<InputManager>(m_window);

   m_runnig = true;
   return true;
}

 void App::shutDown()
 {
    m_renderer.reset();
    m_sim.reset();
    m_gui.reset();
    m_input.reset();

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
    
}

void App::update(float dt)
{

}

void App::render()
{

}