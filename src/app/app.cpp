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

    // Камера видит весь мир (границы задаются в сцене)
    float halfWorldH = 2.4f;
    float fovHalfRad = glm::radians(60.0f / 2.0f);
    float dist = halfWorldH / std::tan(fovHalfRad) * 1.3f; // +30% отступ
    m_camera.setDist(dist);

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

        // Отклик на мышь
        const ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) 
        {
            // ЛКМ drag → orbit
            if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
            {
                float dx = static_cast<float>(m_input.getDeltaX());
                float dy = static_cast<float>(m_input.getDeltaY());
                m_camera.orbit(-dx * 0.35f, dy * 0.35f); 
            }

            // ЦКМ drag → pan
            if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE))
            {
                float dx = static_cast<float>(m_input.getDeltaX());
                float dy = static_cast<float>(m_input.getDeltaY());
                m_camera.pan(dx, -dy);
            }

            // Колёсико → zoom
            if (io.MouseWheel != 0.0f)
            {
                m_camera.zoom(io.MouseWheel * 0.6f);
            }
        }

        m_gui.buildUI(m_state, m_camera, cmd);

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
     m_renderer.updateCamera(m_camera); // Устанавливаем uView + uProj

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

    if (cmd.resetCamera) {
        m_camera.reset();
    }
}