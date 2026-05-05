#include "settingsPanel.h"

#include "app/appState.h"
#include "app/appCommands.h"
#include "render/Camera3D.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>

void SettingsPanel::drawSectionHeader(const char* label)
{
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "%s", label);
    ImGui::Spacing();
}

void SettingsPanel::draw(const AppState& state, const Camera3D& camera, AppCommands& commands)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;

    const float visibleEdge = kPanelW * m_anim;
    const float triggerEdge = std::max(kTriggerX, visibleEdge);
    const bool mouseNear = io.MousePos.x < triggerEdge;

    const bool imguiActive = ImGui::IsAnyItemActive(); // слайдер зажат, кнопка держится
    const bool popupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel); // любой popup открыт 

    const bool keepOpen = mouseNear || imguiActive || popupOpen;
    const float target  = keepOpen ? 1.0f : 0.0f;

    if (popupOpen)
        m_anim = 1.0f;
    else
        m_anim += (target - m_anim) * (1.0f - std::exp(-kAnimSpeed * dt));

    if (m_anim < 0.95f) drawEdgeHint(io, mouseNear);
    if (m_anim < 0.002f) return;

    ImGui::SetNextWindowPos(ImVec2(-kPanelW + kPanelW * m_anim, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, io.DisplaySize.y), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGui::Begin("##SettingsPanel", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoResize        |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing
    );

    // Accent bar слева
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    dl->AddRectFilledMultiColor(
        wp, ImVec2(wp.x + 3.0f, wp.y + ws.y),
        IM_COL32(60,120,200,255), IM_COL32(60,120,200,255),
        IM_COL32(40, 80,160,255), IM_COL32(40, 80,160,255)
    );

    ImGui::Spacing();
    {
        const float tw = ImGui::CalcTextSize("Settings").x;
        ImGui::SetCursorPosX((kPanelW - tw) * 0.5f);
        ImGui::TextColored(ImVec4(0.60f, 0.80f, 1.00f, 1.0f), "Settings");
    }
    ImGui::Spacing();
    ImGui::Separator();

    // ── Блоки в фиксированном порядке ────────────────────────────
    auto sep = [](){ ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); };

    sep(); drawPbfSection(commands);
    sep(); drawRuntimeSection(commands);
    sep(); drawRenderSection(commands);
    sep(); drawSimSection(state, commands);
    sep(); drawCameraSection(camera, commands);
    sep(); drawControlsSection();
    sep(); drawConstantsSection();

    ImGui::Spacing();
    ImGui::End();
}

//  Хинт у левого края (виден когда панель убрана)
void SettingsPanel::drawEdgeHint(const ImGuiIO& io, bool mouseNear)
{
    const float baseAlpha = 1.0f - m_anim;
    if (baseAlpha < 0.01f) return;

    // Пульсация для привлечения внимания
    const float time = (float)ImGui::GetTime();
    const float pulse = 0.85f + 0.15f * sinf(time * 2.0f);
    const float alpha = baseAlpha * pulse;

    // Цвет меняется при наведении
    const ImVec4 normalColor = ImVec4(0.70f, 0.85f, 1.00f, alpha);
    const ImVec4 hoverColor = ImVec4(0.90f, 0.95f, 1.00f, alpha);
    const ImVec4 textColor = mouseNear ? hoverColor : normalColor;

    const float centerY = io.DisplaySize.y * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(0.0f, centerY - kHintH * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kHintW, kHintH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(alpha * 0.7f);

    ImGui::Begin("##SettingsHint", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings
    );

    // Кастомная отрисовка закругленного таба
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // Градиентный фон таба
    drawList->AddRectFilledMultiColor(
        winPos,
        ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(30, 60, 100, (int)(alpha * 180)),
        IM_COL32(40, 80, 130, (int)(alpha * 200)),
        IM_COL32(40, 80, 130, (int)(alpha * 200)),
        IM_COL32(30, 60, 100, (int)(alpha * 180))
    );

    // Accent линия справа
    drawList->AddRectFilled(
        ImVec2(winPos.x + winSize.x - 2.0f, winPos.y),
        ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(60, 120, 200, (int)(alpha * 255))
    );

    // Иконка шеврона
    ImGui::SetCursorPosX(kHintW * 0.5f - 8.0f);
    ImGui::SetCursorPosY(kHintH * 0.5f - 8.0f);
    ImGui::TextColored(textColor, ">>");

    ImGui::End();
}

//  PBF секция
void SettingsPanel::drawPbfSection(AppCommands& commands)
{
    const float innerW = kPanelW - 20.0f;

    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "PBF Fluid");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##RestDensity", &m_restDensity,
                           300.0f, 1000.0f, "Rest density: %.0f"))
    {
        commands.hasSetRestDensity = true;
        commands.restDensityValue  = m_restDensity;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Rest density of the fluid\nLower = sparse, higher = dense");

    ImGui::Spacing();

    if (ImGui::Checkbox("Artificial pressure", &m_artPressure))
    {
        commands.hasSetArtPressure  = true;
        commands.artPressureEnabled = m_artPressure;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reduces particle clustering\nat low density regions");
    
    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##artificialPressureK", &m_artificialPressureK, 0.0005f, 0.005f, "artificialPressureK: %.4f")) 
    {
        commands.hasSetArtificialPressureK = true;
        commands.artificialPressureK = m_artificialPressureK;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Artificial pressure strength.\n");

    ImGui::Spacing();
    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##Vorticity \xce\xb5", &m_vorticityEpsilon, 0.0f, 1.0f, "Vorticity: %.2f")) 
    {
        commands.hasSetVorticity = true;
        commands.vorticityEpsilon = m_vorticityEpsilon;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Vorticity Confinement strength.\n"
                        "0 = off\n"
                        "Restores rotational detail lost by numerical damping.");

    ImGui::Spacing();
    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##XSPH Viscosity", &m_xsphViscosity, 0.0f, 0.6f, "Viscosity: %.2f")) 
    {
        commands.hasSetXSPH = true;
        commands.xsphViscosity = m_xsphViscosity;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("XSPH velocity smoothing.\n0 = off, 0.02 typical.\n"
                          "Blends each particle's velocity toward its neighbours.\n"
                          "Makes the fluid look cohesive, removes jittering.");
}

//  Секция управления симуляцией
void SettingsPanel::drawSimSection(const AppState& state, AppCommands& commands)
{
    const float innerW = kPanelW - 20.0f;

    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Simulation");
    ImGui::Spacing();

    m_backendType = state.backendType;

    const char* backendNames[] = {
        "CPU",
        "CUDA"
    };

    int backendIndex = (m_backendType == SimulationBackendType::CUDA) ? 1 : 0;

    ImGui::SetNextItemWidth(innerW);
    if (ImGui::Combo("Backend", &backendIndex, backendNames, 2))
    {
        m_backendType = (backendIndex == 1)
            ? SimulationBackendType::CUDA
            : SimulationBackendType::CPU;

        commands.hasSetBackend = true;
        commands.backendType = m_backendType;
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "CPU backend uses host Particles3D and fallback VBO upload.\n"
            "CUDA backend uses CUDA kernels and CUDA-OpenGL interop."
        );
    }

    ImGui::Spacing();

    bool pausedLocal = state.paused;
    if (ImGui::Checkbox("Paused", &pausedLocal))
    {
        commands.hasSetPaused   = true;
        commands.setPausedValue = pausedLocal;
    }

    ImGui::Spacing();

    const float btnW = (kPanelW - 28.0f) * 0.5f;

    if (ImGui::Button("Step once", ImVec2(btnW, 0)))
        commands.stepOnce = true;

    ImGui::SameLine();

    if (ImGui::Button("Reset (R)", ImVec2(btnW, 0)))
        commands.reset = true;
}

void SettingsPanel::drawControlsSection()
{
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Controls");
    ImGui::Spacing();

    ImGui::TextDisabled("LMB");
    ImGui::SameLine(80.0f);
    ImGui::Text("Rotate camera");

    ImGui::TextDisabled("RMB");
    ImGui::SameLine(80.0f);
    ImGui::Text("Rotate vessel");

    ImGui::TextDisabled("MMB");
    ImGui::SameLine(80.0f);
    ImGui::Text("Pan camera");

    ImGui::TextDisabled("Wheel");
    ImGui::SameLine(80.0f);
    ImGui::Text("Zoom");

    ImGui::Spacing();

    ImGui::TextDisabled("Space");
    ImGui::SameLine(80.0f);
    ImGui::Text("Pause");

    ImGui::TextDisabled("Right");
    ImGui::SameLine(80.0f);
    ImGui::Text("Step once");

    ImGui::TextDisabled("R");
    ImGui::SameLine(80.0f);
    ImGui::Text("Reset");
}

//  Секция камеры
void SettingsPanel::drawCameraSection(const Camera3D& camera, AppCommands& commands)
{
    const float innerW = kPanelW - 20.0f;

    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Camera");
    ImGui::Spacing();

    const auto eye = camera.getEye();
    ImGui::Text("Eye: (%.1f, %.1f, %.1f)", eye.x, eye.y, eye.z);

    ImGui::Text("Yaw: %.0f°  Pitch: %.0f°",
        camera.getYaw(), camera.getPitch());

    ImGui::Spacing();

    const float btnW = kPanelW - 28.0f;
    if (ImGui::Button("Reset View", ImVec2(btnW, 0)))
        commands.resetCamera = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset camera to default position");
}

void SettingsPanel::drawRuntimeSection(AppCommands& commands)
{
    const float innerW = kPanelW - 20.0f;

    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Runtime");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat3("Gravity", m_gravity, -9.81f, 9.81f))
    {
        commands.hasSetGravity = true;
        commands.gravityX = m_gravity[0];
        commands.gravityY = m_gravity[1];
        commands.gravityZ = m_gravity[2];
    }

    ImGui::Spacing();

    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##MaxSpeed", &m_maxSpeed,
                           1.0f, 16.0f, "Max speed: %.1f"))
    {
        commands.hasSetMaxSpeed = true;
        commands.maxSpeed = m_maxSpeed;
    }

    ImGui::Spacing();

    ImGui::SetNextItemWidth(innerW);
    bool wallChanged = false;

    wallChanged |= ImGui::SliderFloat("##WallRestitution", &m_wallRestitution, 0.0f, 1.0f, "Restitution: %.2f");
    
    ImGui::SetNextItemWidth(innerW);
    wallChanged |= ImGui::SliderFloat("##WallFriction", &m_wallFriction, 0.0f, 1.0f, "Friction: %.2f");

    if (wallChanged)
    {
        commands.hasSetWallResponse = true;
        commands.wallRestitution = m_wallRestitution;
        commands.wallFriction = m_wallFriction;
    }

    ImGui::Spacing();

    if (ImGui::Checkbox("Baffle pair filtering", &m_baffleFiltering))
    {
        commands.hasSetBaffleFiltering = true;
        commands.baffleFilteringEnabled = m_baffleFiltering;
    }
}

void SettingsPanel::drawRenderSection(AppCommands& commands)
{
    const float innerW = kPanelW - 20.0f;

    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Rendering");
    ImGui::Spacing();

    const char* colorModes[] = {
        "Speed gradient",
        "Phase colors"
    };

    ImGui::SetNextItemWidth(innerW);
    if (ImGui::Combo("Color mode", &m_particleColorMode, colorModes, 2))
    {
        commands.hasSetParticleColorMode = true;
        commands.particleColorMode = m_particleColorMode;
    }

    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##MaxGradSpeed", &m_maxGradSpeed,
                           0.1f, 20.0f, "Speed color scale: %.1f"))
    {
        commands.hasSetMaxGradSpeed = true;
        commands.maxGradSpeed = m_maxGradSpeed;
    }

    bool colorsChanged = false;
    colorsChanged |= ImGui::ColorEdit4("Phase 0", m_phase0Color);
    colorsChanged |= ImGui::ColorEdit4("Phase 1", m_phase1Color);

    if (colorsChanged)
    {
        commands.hasSetPhaseColors = true;

        commands.phase0Color = glm::vec4(
            m_phase0Color[0],
            m_phase0Color[1],
            m_phase0Color[2],
            m_phase0Color[3]
        );

        commands.phase1Color = glm::vec4(
            m_phase1Color[0],
            m_phase1Color[1],
            m_phase1Color[2],
            m_phase1Color[3]
        );
    }
}

void SettingsPanel::drawConstantsSection()
{
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Fixed constants");
    ImGui::Spacing();

    ImGui::TextDisabled("Particle radius: %.4f", Config::particleRadius);
    ImGui::TextDisabled("Smoothing radius: %.4f", Config::smoothingRadius);
    ImGui::TextDisabled("Lambda epsilon: %.1f", Config::epsilon);
    ImGui::TextDisabled("Iterations: %d", Config::iterations);
    ImGui::TextDisabled("Artificial deltaQ: %.3f", Config::artificialPressureDeltaQ);
    ImGui::TextDisabled("Particle mass: %.3f", Config::particleMass);
    ImGui::TextDisabled("dt: %.5f", Config::dt);
}
