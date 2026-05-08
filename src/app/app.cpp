#include "app.h"

#include <stdexcept>
#include <iostream>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common/config.h"
#include "scene/ScenePresets.h"

#include "bench/BenchmarkRunner.h"
#include "scene/benchmarkScenes.h"
#include "bench/CsvWriter.h"

namespace
{
    constexpr float kCameraOrbitSensitivity = 0.35f;      // град/пиксель
    constexpr float kVesselRotationSensitivity = 0.20f;   // град/пиксель

    glm::mat4 buildVesselModelMatrix(const VesselBoundary& vessel)
    {
        const glm::mat4 T1 = glm::translate(glm::mat4(1.0f), vessel.pivot);
        const glm::mat4 R = glm::mat4_cast(vessel.orientation);
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

bool App::initialize(int argc, char** argv)
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

    parseBenchmarkArgs(argc, argv);

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

    if (m_benchmarkMode) 
    {
        // Один рендер-кадр для инициализации OpenGL-состояния (ImGui, VAO и т.д.)
        glfwPollEvents();
        m_gui.beginFrame();
        AppCommands cmd; 
        m_gui.buildUI(m_state, m_camera, cmd);
        render();
        m_gui.endFrame();
        glfwSwapBuffers(m_window);

        runBenchmarkMode();
        return; // выходим сразу, без mainLoop
    }

    mainLoop();
 }

void App::mainLoop()
{
    const double dt = Config::dt;
    m_previousTime = glfwGetTime();

    bool benchmarkStarted = false;

    while (m_running && !glfwWindowShouldClose(m_window))
    {   
        glfwPollEvents();
        m_input.update();

        double currentTime = glfwGetTime();
        double frameTime = currentTime - m_previousTime;
        m_previousTime = currentTime;
        
        m_gui.setFrameTiming(frameTime);

        AppCommands cmd;

        m_gui.beginFrame();

        if (m_input.justPressed(GLFW_KEY_SPACE)) cmd.togglePause = true;
        if (m_input.justPressed(GLFW_KEY_R)) cmd.reset = true;
        if (m_input.justPressed(GLFW_KEY_RIGHT)) cmd.stepOnce = true;

        const bool pausedForInput = cmd.togglePause ? !m_state.paused : m_state.paused;

        // Отклик на мышь
        // ЛКМ — вращение камеры
        // ПКМ — вращение сосуда
        // ЦКМ — смещение камеры
        // Колёсико — зум
        const ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
        {
            const float dx = static_cast<float>(m_input.getDeltaX());
            const float dy = static_cast<float>(m_input.getDeltaY());

            if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
            {
                m_camera.orbit(-dx * kCameraOrbitSensitivity, dy * kCameraOrbitSensitivity);
            }

            if (!pausedForInput && m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
            {
                rotateVesselFromMouseDrag(dx, dy);
            }

            if (m_input.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE))
            {
                m_camera.pan(dx, -dy);
            }

            if (io.MouseWheel != 0.0f)
            {
                m_camera.zoom(io.MouseWheel * 0.6f);
            }
        }

        m_gui.setParticleCount(m_sim.getParticles().count);
        m_gui.buildUI(m_state, m_camera, cmd);

        applyCommands(cmd);

        double startPhysicsTime = glfwGetTime();
        if (!m_state.paused || cmd.stepOnce)
        {
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

    if (cmd.hasSetBackend)
    {
        switchBackend(cmd.backendType);
    }

    if (cmd.hasSetPaused) m_state.paused = cmd.setPausedValue;
    
    if (cmd.hasSetRestDensity)
        Config::restDensity = cmd.restDensityValue;

    if (cmd.hasSetArtPressure) 
    {
        m_state.artPressureEnabled = cmd.artPressureEnabled;
        m_sim.setArtificialPressureK(cmd.artPressureEnabled ? Config::artificialPressureK : 0.0f);
    }

    if (cmd.hasSetVorticity)
    {
        Config::vorticityEpsilon = cmd.vorticityEpsilon;
        m_sim.setVorticityEpsilon(Config::vorticityEpsilon);
    }

    if (cmd.hasSetXSPH)
    {
        Config::xsphViscosity = cmd.xsphViscosity;
        m_sim.setXsphViscosity(Config::xsphViscosity);
    }

    if (cmd.hasSetScene)
    {
        m_state.activeSceneIndex = cmd.sceneIndex;
        m_activeSceneDesc = ScenePresets::getByIndex(cmd.sceneIndex);

        reloadActiveScene(true);

        syncGuiWithConfig();
    }

    if (cmd.reset)
    {
        reloadActiveScene(true);

        syncGuiWithConfig();
    }

    if (cmd.resetCamera) {
        m_camera.reset();
    }

    if (cmd.hasSetGravity)
    {
        Config::gravityX = cmd.gravityX;
        Config::gravityY = cmd.gravityY;
        Config::gravityZ = cmd.gravityZ;

        m_sceneRuntime.gravityWorld = {
            cmd.gravityX,
            cmd.gravityY,
            cmd.gravityZ
        };
    }

    if (cmd.hasSetArtificialPressureK)
    {
        Config::artificialPressureK = cmd.artificialPressureK;

        if (m_state.artPressureEnabled)
            m_sim.setArtificialPressureK(Config::artificialPressureK);
    }

    if (cmd.hasSetMaxSpeed)
    {
        Config::maxSpeed = cmd.maxSpeed;
    }

    if (cmd.hasSetWallResponse)
    {
        Config::wallRestitution = cmd.wallRestitution;
        Config::wallFriction = cmd.wallFriction;
    }

    if (cmd.hasSetBaffleFiltering)
    {
        Config::enableBafflePairFiltering = cmd.baffleFilteringEnabled;
    }

    if (cmd.hasSetParticleColorMode)
    {
        Config::particleColorMode = cmd.particleColorMode;
    }

    if (cmd.hasSetMaxGradSpeed)
    {
        Config::maxGradSpeed = cmd.maxGradSpeed;
        m_renderer.setMaxSpeed(Config::maxGradSpeed);
    }

    if (cmd.hasSetPhaseColors)
    {
        Config::phase0Color = cmd.phase0Color;
        Config::phase1Color = cmd.phase1Color;
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
	glfwSwapInterval(0); // v-sync

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
    m_gui.setBackendType(m_backendType);

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
    m_state.activeSceneIndex = idx;
    m_state.backendType = m_backendType;

    m_activeSceneDesc = ScenePresets::getByIndex(idx);

    reloadActiveScene(true);

    return true;
}

void App::syncGuiWithConfig()
{
    m_gui.setRestDensity(Config::restDensity);
    m_gui.setVorticityEpsilon(Config::vorticityEpsilon);
    m_gui.setXsphViscosity(Config::xsphViscosity);
    m_gui.setBackendType(m_backendType);

    m_gui.setGravity(Config::gravityX, Config::gravityY, Config::gravityZ);
    m_gui.setMaxSpeed(Config::maxSpeed);
    m_gui.setWallResponse(Config::wallRestitution, Config::wallFriction);
    m_gui.setArtificialPressureK(Config::artificialPressureK);
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

void App::reloadActiveScene(bool resetRuntimeState)
{
    m_sim.loadScene(m_activeSceneDesc);
    m_renderer.uploadVesselWireframe(m_activeSceneDesc.vessel.wireframe);

    if (resetRuntimeState)
        resetSceneRuntimeState();
    else
        applySceneRuntimeState();

    syncBackendWithConfig();
    setupInteropForCurrentBackend();
}

void App::setupInteropForCurrentBackend()
{
    m_interopEnabled = false;

    if (m_backendType != SimulationBackendType::CUDA)
        return;

    const int n = m_sim.getParticles().count;

    m_renderer.ensureInstanceBufferSize(n);
    m_interopEnabled = m_sim.setupInterop(m_renderer.getInstanceVBO());
}

void App::syncBackendWithConfig()
{
    m_sim.setArtificialPressureK(
        m_state.artPressureEnabled ? Config::artificialPressureK : 0.0f
    );

    m_sim.setVorticityEpsilon(Config::vorticityEpsilon);
    m_sim.setXsphViscosity(Config::xsphViscosity);

    m_renderer.setMaxSpeed(Config::maxGradSpeed);
}

void App::switchBackend(SimulationBackendType type)
{
    if (m_backendType == type)
        return;

    m_interopEnabled = false;

    m_backendType = type;
    m_state.backendType = type;

    m_sim.switchTo(type);

    m_gui.setBackendType(type);

    // Сцену перезагружаем, но runtime-состояние сосуда сохраняем:
    // это удобно для сравнения CPU/CUDA на одной ориентации.
    reloadActiveScene(false);
}

void App::parseBenchmarkArgs(int argc, char** argv) 
{
    for (int i = 1; i < argc; ++i) 
    {
        std::string arg = argv[i];
        auto next = [&]() -> std::string 
        {
            return (i + 1 < argc) ? argv[++i] : "";
        };

        if (arg == "--benchmark") 
        { 
            m_benchmarkMode = true;                             
            m_benchmarkCfg.testName = next(); 
        }
        else if (arg == "--backend") 
        { 
            auto v = next();
            m_benchmarkCfg.backend = (v == "cpu") ? SimulationBackendType::CPU : SimulationBackendType::CUDA; 
        }
        else if (arg == "--particles") { m_benchmarkCfg.targetParticles = std::stoi(next()); }
        else if (arg == "--iterations") { m_benchmarkCfg.iterations = std::stoi(next()); }
        else if (arg == "--scene") { m_benchmarkCfg.sceneName = next(); }
        else if (arg == "--warmup") { m_benchmarkCfg.warmupFrames = std::stoi(next()); }
        else if (arg == "--frames") { m_benchmarkCfg.measureFrames = std::stoi(next()); }
        else if (arg == "--repeats") { m_benchmarkCfg.repeats = std::stoi(next()); }
        else if (arg == "--output") { m_benchmarkCfg.outputPath = next(); }
    }
}

void App::runBenchmarkMode() 
{
    m_sceneRuntime = SceneRuntimeState{};
    m_state.paused = false;
    switchBackend(m_benchmarkCfg.backend);

    // --- выбор сцены ---
    SceneDescription scene;
    const auto& sn = m_benchmarkCfg.sceneName;
    if (sn == "benchmark_box")  scene = BenchmarkScenes::makeBox(m_benchmarkCfg.targetParticles);
    else if (sn == "benchmark_dambreak") scene = BenchmarkScenes::makeDamBreak(m_benchmarkCfg.targetParticles);
    else 
    {
        int idx = 0;
        for (int i = 0; i < ScenePresets::count(); ++i)
            if (std::string(ScenePresets::names()[i]) == sn) { idx = i; break; }
        scene = ScenePresets::getByIndex(idx);
    }
    m_activeSceneDesc = scene;
    m_renderer.uploadVesselWireframe(scene.vessel.wireframe);
    // ---

    BenchmarkRunner runner;
    for (int rep = 0; rep < m_benchmarkCfg.repeats; ++rep) 
    {
        m_sceneRuntime = SceneRuntimeState{};
        m_sim.loadScene(m_activeSceneDesc);
        m_benchmarkCfg.actualParticles = m_sim.getParticles().count;
        m_sim.setIterations(m_benchmarkCfg.iterations);
        applySceneRuntimeState();
        setupInteropForCurrentBackend();

        std::cout << "[Benchmark] rep " << rep+1 << "/" << m_benchmarkCfg.repeats
                  << "  particles=" << m_benchmarkCfg.actualParticles << "  warming up...\n";

        // --- Warmup с рендером каждые 10 кадров ---
        m_sim.setBenchmarkSkipReadback(m_benchmarkCfg.skipReadback);
        for (int i = 0; i < m_benchmarkCfg.warmupFrames; ++i) 
        {
            m_sim.update(m_benchmarkCfg.dt);
            if (i % 10 == 0) 
            {
                glfwPollEvents();
                m_gui.beginFrame();
                AppCommands cmd;
                m_gui.buildUI(m_state, m_camera, cmd);
                render();
                m_gui.endFrame();
                glfwSwapBuffers(m_window);
            }
        }

        // --- Measurement с рендером каждые 10 кадров ---
        const bool perFrame = (m_benchmarkCfg.testName == "perf_stability");

        std::vector<FrameTiming> timings;
        timings.reserve(m_benchmarkCfg.measureFrames);

        for (int i = 0; i < m_benchmarkCfg.measureFrames; ++i)
        {
            m_sim.update(m_benchmarkCfg.dt);
            timings.push_back(m_sim.getLastFrameTiming());

            if (perFrame)
            {
                BenchmarkResult fr;
                fr.testName = m_benchmarkCfg.testName;
                fr.backend = (m_benchmarkCfg.backend == SimulationBackendType::CUDA) ? "cuda" : "cpu";
                fr.sceneName = m_benchmarkCfg.sceneName;
                fr.actualParticles = m_benchmarkCfg.actualParticles;
                fr.iterations = m_benchmarkCfg.iterations;
                fr.repeatId = i;   // ← номер кадра, а не повтора

                const FrameTiming& ft = timings.back();
                fr.avgStepMs = ft.totalStepMs;
                fr.medianStepMs = ft.totalStepMs;
                fr.p95StepMs = ft.totalStepMs;
                fr.stdStepMs = 0.0;
                fr.physicsFps = ft.totalStepMs > 0.0 ? 1000.0 / ft.totalStepMs : 0.0;
                fr.avgPredictMs = ft.predictMs;
                fr.avgNeighborMs = ft.neighborMs;
                fr.avgSolverMs = ft.solverMs;
                fr.avgVelocityCorrectMs = ft.velocityCorrectMs;

                CsvWriter::append(m_benchmarkCfg.outputPath, fr);
            }

            if (i % 10 == 0)
            {
                glfwPollEvents();
                m_gui.beginFrame();
                AppCommands cmd;
                m_gui.buildUI(m_state, m_camera, cmd);
                render();
                m_gui.endFrame();
                glfwSwapBuffers(m_window);
            }
        }

        m_sim.setBenchmarkSkipReadback(false);

        if (!perFrame)
        {
            BenchmarkResult result = runner.aggregate(timings, m_benchmarkCfg, rep);
            result.testName = m_benchmarkCfg.testName;
            result.backend = (m_benchmarkCfg.backend == SimulationBackendType::CUDA) ? "cuda" : "cpu";
            result.sceneName = m_benchmarkCfg.sceneName;
            result.actualParticles = m_benchmarkCfg.actualParticles;
            CsvWriter::append(m_benchmarkCfg.outputPath, result);

            std::cout << "  rep " << rep + 1
                    << "  particles=" << result.actualParticles
                    << "  avg=" << result.avgStepMs << " ms\n";
        }
        else
        {
            std::cout << "  [perf_stability] rep " << rep + 1
                    << "  записано " << timings.size() << " кадров\n";
        }
    }
}
