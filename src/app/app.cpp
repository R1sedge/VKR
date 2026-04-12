#include "app.h"

#include <stdexcept>
#include <iostream>

#include "common/config.h"
#include "scene/ScenePresets.h"

App::App()
    : m_renderer(Config::windowWidth, Config::windowHeight, nullptr),
      m_backendType(SimulationBackendType::CUDA),
      m_sim(m_backendType),
      m_gui(),
      m_input(),
      m_state()
{
}

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

    // Register scroll callback for mouse wheel
    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
        if (input) input->addScrollDelta(yoffset);
    });
    glfwSetWindowUserPointer(m_window, &m_input);

    if (!gladLoadGL())
	{   
        std::cerr << "GLAD init failed\n";
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
        return false;
    }

    m_renderer.setWindow(m_window);
    m_input.setWindow(m_window);

    m_gui.initialize(m_window);
    m_gui.setSimulationDt(Config::dt);
    m_gui.setRestDensity(Config::restDensity);
    m_gui.setVorticityEpsilon(Config::vorticityEpsilon);
    m_gui.setXsphViscosity(Config::xsphViscosity);

    m_gui.setSceneIndex(0);

    float halfWorldW = Config::windowWidth / (2.0f * Config::pixelsPerUnits);
    float halfWorldH = Config::windowHeight / (2.0f * Config::pixelsPerUnits);

    m_sim.setWorldBounds(-halfWorldW, halfWorldW, 
                         -halfWorldH, halfWorldH);

    m_running = true;
    
    m_sim.loadScene(ScenePresets::getByIndex(0));

    if (m_backendType == SimulationBackendType::CUDA)
    {
        int n = m_sim.getParticles().count;
        m_renderer.ensureInstanceBufferSize(n);
        m_interopEnabled = m_sim.setupInterop(m_renderer.getInstanceVBO());
    }

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
    m_running = false;
 }

 void App::run()
 {
    if(!m_running) return;
    mainLoop();
 }

void App::mainLoop()
{
    const double dt = Config::dt;
    const double maxFrameTime = 0.25;

    m_previousTime = glfwGetTime();

    while (m_running && !glfwWindowShouldClose(m_window))
    {   
        glfwPollEvents();
        m_input.update();

        double currentTime = glfwGetTime();
        double frameTime = currentTime - m_previousTime;
        m_previousTime = currentTime;

        if (frameTime > maxFrameTime) 
            frameTime = maxFrameTime;

        m_gui.setFrameTiming(frameTime);

        AppCommands cmd;

        m_gui.beginFrame();

        if (m_input.justPressed(GLFW_KEY_SPACE)) cmd.togglePause = true;
        if (m_input.justPressed(GLFW_KEY_R)) cmd.reset = true;

        // Mouse interaction (only if not hovering UI AND in Force mode)
        const ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && m_state.interactionMode == 0) {
            // Adjust radius with scroll
            double scroll = m_input.getScrollDelta();
            if (scroll != 0.0) {
                m_state.mouseForceRadius += static_cast<float>(scroll) * 0.1f;
                if (m_state.mouseForceRadius < 0.5f) m_state.mouseForceRadius = 0.5f;
                if (m_state.mouseForceRadius > 3.0f) m_state.mouseForceRadius = 3.0f;
                m_input.resetScrollDelta();
            }

            // Apply forces
            if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) ||
                m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT) ||
                m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE))
            {
                // Get actual window size (handles resize)
                int windowWidth, windowHeight;
                glfwGetWindowSize(m_window, &windowWidth, &windowHeight);

                // Convert screen to world coordinates
                double screenX = m_input.getMouseX();
                double screenY = m_input.getMouseY();

                float worldX = static_cast<float>((screenX - windowWidth * 0.5) / Config::pixelsPerUnits);
                float worldY = static_cast<float>(-(screenY - windowHeight * 0.5) / Config::pixelsPerUnits);

                cmd.hasMouseForce = true;
                cmd.mouseForceWorldX = worldX;
                cmd.mouseForceWorldY = worldY;
                cmd.mouseForceRadius = m_state.mouseForceRadius;

                if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
                    cmd.mouseForceType = 0;  // Repulsion
                    cmd.mouseForceStrength = 2.0f;
                }
                else if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
                    cmd.mouseForceType = 1;  // Attraction
                    cmd.mouseForceStrength = 1.0f;
                }
                else if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE)) {
                    cmd.mouseForceType = 2;  // Vortex
                    cmd.mouseForceStrength = 0.3f;
                }
            }
        }

        m_gui.buildUI(m_state, cmd);

        applyCommands(cmd);

        double startPhysicsTime = glfwGetTime();
        if (!m_state.paused || cmd.stepOnce)
        {
           // Apply mouse forces only when simulation is running
           if (cmd.hasMouseForce) {
               m_sim.applyMouseForce(cmd.mouseForceWorldX, cmd.mouseForceWorldY,
                                    cmd.mouseForceRadius, cmd.mouseForceStrength,
                                    cmd.mouseForceType);
           }
           update(dt);
        }
        double endPhysicsTime = glfwGetTime();

        double startRenderTime = glfwGetTime();
        render();
        double endRenderTime = glfwGetTime();

        m_gui.setPhysicsTiming(endPhysicsTime - startPhysicsTime);
        m_gui.setRenderTiming(endRenderTime - startRenderTime);
        
        m_gui.endFrame();

        glfwSwapBuffers(m_window);
    }
}

void App::update(float dt)
{
    m_sim.update(dt);
}

void App::render()
{  
    if (m_interopEnabled) {
        // GPU-путь: данные уже в VBO, никакого memcpy
        m_renderer.renderFrameInterop(m_sim.getParticles().count);
    } else {
        // CPU-путь: старый код
        m_renderer.renderFrame(m_sim.getParticles());
    }
}

void App::applyCommands(AppCommands& cmd)
{
    if (cmd.togglePause) m_state.paused = !m_state.paused;
    if (cmd.hasSetPaused) m_state.paused = cmd.setPausedValue;

    if (cmd.hasSetRestDensity)
        Config::restDensity = cmd.restDensityValue;

    if (cmd.hasSetArtPressure) 
    {
        m_state.artPressureEnabled = cmd.artPressureEnabled;
        m_sim.setArtificialPressureK(cmd.artPressureEnabled ? Config::artificialPressureK : 0.0f);
    }

    if (cmd.hasSetVorticity)
        m_sim.setVorticityEpsilon(cmd.vorticityEpsilon);

    if (cmd.hasSetXSPH)
        m_sim.setXsphViscosity(cmd.xsphViscosity);

    if (cmd.hasSetScene) 
    {
        m_state.activeSceneIndex = cmd.sceneIndex;
        m_sim.loadScene(ScenePresets::getByIndex(cmd.sceneIndex));
        if (m_interopEnabled) 
        {
            m_renderer.ensureInstanceBufferSize(m_sim.getParticles().count);
            m_sim.resetInterop(m_renderer.getInstanceVBO());
        }
    }

    if (cmd.reset)
    {
        m_sim.loadScene(ScenePresets::getByIndex(m_state.activeSceneIndex));
        if (m_interopEnabled)
        {
            m_renderer.ensureInstanceBufferSize(m_sim.getParticles().count);
            m_sim.resetInterop(m_renderer.getInstanceVBO());
        }
    }

    if (cmd.hasSetInteractionMode) {
        m_state.interactionMode = cmd.interactionMode;
    }

    if (cmd.hasSetMouseForceRadius) {
        m_state.mouseForceRadius = cmd.mouseForceRadius;
    }
}