#include "app.h"

#include <stdexcept>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common/config.h"
#include "scene/ScenePresets.h"

namespace
{
    constexpr float kCameraOrbitSensitivity = 0.35f;      // град/пиксель
    constexpr float kVesselRotationSensitivity = 0.20f;   // град/пиксель

    glm::mat4 buildVesselModelMatrix(const VesselBoundary& vessel)
    {
        const glm::mat4 T1 = glm::translate(glm::mat4(1.0f), vessel.pivot);
        const glm::mat4 R  = glm::mat4_cast(vessel.orientation);
        const glm::mat4 T2 = glm::translate(glm::mat4(1.0f), -vessel.pivot);
        return T1 * R * T2;
    }

    glm::mat4 buildReferenceGridModelMatrix(float y, float extent)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, y, 0.0f));
        model = glm::scale(model, glm::vec3(extent, 1.0f, extent));
        return model;
    }
}

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
    // 1. Инициализация GLFW (оконная система, события)
    if (!initializeGLFW())
    {
        std::cerr << "[App] GLFW initialization failed\n";
        return false;
    }

    // 2. Создание окна + OpenGL-контекст (glfwMakeContextCurrent внутри)
    if (!createWindow())
    {
        std::cerr << "[App] Window creation failed\n";
        return false;
    }

    // 3. Загрузка указателей OpenGL 4.5 через GLAD
    //    Обязательно ПОСЛЕ glfwMakeContextCurrent
    if (!initializeGLAD())
    {
        std::cerr << "[App] GLAD initialization failed\n";
        return false;
    }

    // 4. Привязка m_window к Renderer и InputManager
    //    Renderer.setWindow → хранит указатель, InputManager → регистрирует GLFW-колбэки
    if (!setWindow())
    {
        std::cerr << "[App] setWindow failed\n";
        return false;
    }

    // 5. ImGui: создание контекста, привязка к GLFW + OpenGL backend
    if (!initializeIMGUI(0))
    {
        std::cerr << "[App] ImGui initialization failed\n";
        return false;
    }

    // 6. Камера: вычисление начальной дистанции под FOV и размер мира
    if (!initializeCamera())
    {
        std::cerr << "[App] Camera initialization failed\n";
        return false;
    }

    // 7. Загрузка стартовой сцены (индекс 0)
    //    Внутри: sim.loadScene + CUDA-GL interop setup (ensureInstanceBufferSize, setupInterop)
    if (!initializeScene(0))
    {
        std::cerr << "[App] Scene initialization failed\n";
        return false;
    }

    m_running = true;
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
            const float dx = static_cast<float>(m_input.getDeltaX());
            const float dy = static_cast<float>(m_input.getDeltaY());

            switch (m_state.interactionMode)
            {
                case InteractionModeCameraControl:
                {
                    if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
                    {
                        m_camera.orbit(-dx * kCameraOrbitSensitivity,
                                        dy * kCameraOrbitSensitivity);
                    }

                    if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE))
                    {
                        m_camera.pan(dx, -dy);
                    }

                    if (io.MouseWheel != 0.0f)
                    {
                        m_camera.zoom(io.MouseWheel * 0.6f);
                    }
                    break;
                }

                case InteractionModeVesselRotation:
                {
                    if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
                    {
                        rotateVesselFromMouseDrag(dx, dy);
                    }
                    break;
                }

                case InteractionModeForceApplication:
                {
                    // Здесь камера мышью не управляется.
                    // Сама логика применения силы остаётся отдельной.
                    break;
                }

                default:
                    break;
            }
        }

        m_gui.setParticleCount(m_sim.getParticles().count);
        m_gui.buildUI(m_state, m_camera, cmd);

        applyCommands(cmd);

        double startPhysicsTime = glfwGetTime();
        if (!m_state.paused || cmd.stepOnce)
        {
           const int effectiveInteractionMode = cmd.hasSetInteractionMode ? cmd.interactionMode : m_state.interactionMode;

        if (cmd.hasMouseForce && effectiveInteractionMode == InteractionModeForceApplication)
        {
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
    m_renderer.updateCamera(m_camera);

    if (m_interopEnabled)
        m_renderer.renderFrameInterop(m_sim.getParticles().count);
    else
        m_renderer.renderFrame(m_sim.getParticles());

    VesselBoundary vessel = m_activeSceneDesc.vessel;
    vessel.orientation = m_sceneRuntime.vesselOrientation;

    const glm::mat4 vesselModel = buildVesselModelMatrix(vessel);
    m_renderer.renderVesselWireframe(vesselModel, m_camera);

    const float vesselRadius = std::max(1.0f, m_activeSceneDesc.vessel.computeBoundingRadius());
    const float gridY = -vesselRadius - 0.15f;
    const float gridExtent = vesselRadius * 1.75f;

    const glm::mat4 gridModel = buildReferenceGridModelMatrix(gridY, gridExtent);
    m_renderer.renderReferenceGrid(gridModel, m_camera);
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
        m_activeSceneDesc = ScenePresets::getByIndex(cmd.sceneIndex);

        m_sim.loadScene(m_activeSceneDesc);
        m_renderer.uploadVesselWireframe(m_activeSceneDesc.vessel.wireframe);

        if (m_interopEnabled) 
        {
            m_renderer.ensureInstanceBufferSize(m_sim.getParticles().count);
            m_sim.resetInterop(m_renderer.getInstanceVBO());
        }

        resetSceneRuntimeState();
    }

    if (cmd.reset)
    {
        m_sim.loadScene(m_activeSceneDesc);
        m_renderer.uploadVesselWireframe(m_activeSceneDesc.vessel.wireframe);

        if (m_interopEnabled)
        {
            m_renderer.ensureInstanceBufferSize(m_sim.getParticles().count);
            m_sim.resetInterop(m_renderer.getInstanceVBO());
        }

        resetSceneRuntimeState();
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

bool App::initializeGLFW()
{
    if (!glfwInit())
    {
		std::cerr << "GLFW init failed\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return true;
}

bool App::createWindow()
{
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

    return true;
}

bool App::initializeGLAD()
{
    if (!gladLoadGL())
	{   
        std::cerr << "GLAD init failed\n";
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
        return false;
    }
    
    return true;
}

bool App::setWindow()
{
    m_renderer.setWindow(m_window);
    m_input.setWindow(m_window);
    return true;
}

bool App::initializeIMGUI(int idx)
{
    m_gui.initialize(m_window);
    m_gui.setSimulationDt(Config::dt);
    m_gui.setRestDensity(Config::restDensity);
    m_gui.setVorticityEpsilon(Config::vorticityEpsilon);
    m_gui.setXsphViscosity(Config::xsphViscosity);

    m_gui.setSceneIndex(idx);

    return true;
}

bool App::initializeCamera()
{
    float halfWorldH = 2.4f;
    float fovHalfRad = glm::radians(60.0f / 2.0f);
    float dist = halfWorldH / std::tan(fovHalfRad) * 1.3f; // +30% отступ
    m_camera.setDist(dist);

    return true;
}

bool App::initializeScene(int idx)
{
    m_activeSceneDesc = ScenePresets::getByIndex(idx);

    m_sim.loadScene(m_activeSceneDesc);
    m_renderer.uploadVesselWireframe(m_activeSceneDesc.vessel.wireframe);

    if (m_backendType == SimulationBackendType::CUDA)
    {
        int n = m_sim.getParticles().count;
        m_renderer.ensureInstanceBufferSize(n);
        m_interopEnabled = m_sim.setupInterop(m_renderer.getInstanceVBO());
    }

    resetSceneRuntimeState();
    return true;
}

void App::rotateVesselFromMouseDrag(float dxPixels, float dyPixels)
{
    if (dxPixels == 0.0f && dyPixels == 0.0f)
        return;

    const glm::vec3 cameraUp = glm::normalize(m_camera.getCamUp());
    const glm::vec3 cameraRight = glm::normalize(m_camera.getRight());

    const float yawRad = glm::radians(dxPixels * kVesselRotationSensitivity);
    const float pitchRad = glm::radians(dyPixels * kVesselRotationSensitivity);

    const glm::quat qYaw = glm::angleAxis(yawRad, cameraUp);
    const glm::quat qPitch = glm::angleAxis(pitchRad, cameraRight);

    const glm::quat delta = glm::normalize(qYaw * qPitch);
    const glm::quat nextOrientation = glm::normalize(delta * m_sceneRuntime.vesselOrientation);

    setRuntimeVesselOrientation(nextOrientation);
}

void App::resetSceneRuntimeState()
{
    m_sceneRuntime = SceneRuntimeState{};
    applySceneRuntimeState();
}

void App::applySceneRuntimeState()
{
    m_sim.setVesselOrientation(m_sceneRuntime.vesselOrientation);

    // Потом:
    // Config::gravityX = m_sceneRuntime.gravityWorld.x;
    // Config::gravityY = m_sceneRuntime.gravityWorld.y;
    // Config::gravityZ = m_sceneRuntime.gravityWorld.z;
}

void App::setRuntimeVesselOrientation(const glm::quat& q)
{
    m_sceneRuntime.vesselOrientation = glm::normalize(q);
    applySceneRuntimeState();
}
