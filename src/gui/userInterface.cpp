#include "userInterface.h"
#include "app/appState.h"
#include "app/appCommands.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


UserInterface::~UserInterface()
{
    if (initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}


//  Инициализация ImGui + стиль
bool UserInterface::initialize(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 6.0f;
    style.FrameRounding    = 4.0f;
    style.GrabRounding     = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize  = 1.0f;
    style.FramePadding     = ImVec2(8, 4);
    style.ItemSpacing      = ImVec2(8, 6);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]             = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    c[ImGuiCol_WindowBg]         = ImVec4(0.08f, 0.08f, 0.10f, 0.88f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.20f, 0.20f, 0.28f, 1.00f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.24f, 0.24f, 0.34f, 1.00f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.38f, 0.66f, 1.00f, 1.00f);
    c[ImGuiCol_Button]           = ImVec4(0.20f, 0.40f, 0.70f, 0.80f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.28f, 0.52f, 0.88f, 1.00f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.38f, 0.62f, 1.00f, 1.00f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.38f, 0.70f, 1.00f, 1.00f);
    c[ImGuiCol_Header]           = ImVec4(0.20f, 0.40f, 0.70f, 0.50f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.28f, 0.52f, 0.88f, 0.80f);
    c[ImGuiCol_Separator]        = ImVec4(0.35f, 0.35f, 0.45f, 1.00f);

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) return false;
    if (!ImGui_ImplOpenGL3_Init("#version 450"))      return false;

    initialized = true;
    return true;
}


//  Метрики — из FrameTimer
void UserInterface::setFrameTiming(double frameTimeSeconds)
{
    m_timer.pushFrame(frameTimeSeconds);
}

void UserInterface::setPhysicsTiming(double seconds)
{
    m_timer.pushPhysics(seconds);
}

void UserInterface::setRenderTiming(double seconds)
{
    m_timer.pushRender(seconds);
}


//  Этапы кадра
void UserInterface::beginFrame()
{
    if (!initialized) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UserInterface::buildUI(const AppState& state, AppCommands& commands)
{
    if (!initialized) return;

    m_fps.draw(m_timer);
    m_stats.draw(m_timer, m_simDt);
    m_settings.draw(state, commands);
    m_scenes.draw(state, commands);
}

void UserInterface::endFrame()
{
    if (!initialized) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}