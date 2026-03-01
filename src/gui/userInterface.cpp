#include "userInterface.h"
#include "app/appState.h"
#include "app/appCommands.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

UserInterface::~UserInterface()
{
    if(initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

bool UserInterface::initialize(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Тёмная тема
    ImGui::StyleColorsDark();

    // Привязка ImGui к GLFW
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
        return false;
    
    const char* glsl_version = "#version 450";
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
        return false;
    
    initialized = true;
    return true;
}

void UserInterface::setFrameTiming(double frameTimeSeconds)
{
    float ms = (float)(frameTimeSeconds * 1000);
    float fps;
    if (frameTimeSeconds > 0.0)
        fps = 1 / frameTimeSeconds;
    else
        fps = 0.0f;

    // Кольцевой буфер
    frameTimes[histoyIndex] = ms;
    histoyIndex = (histoyIndex + 1) % historySize;
    if(historyCount < historySize)
        historyCount++;

    // Сглаженные значения
    float sumMs = 0.0f;
    for (int i = 0; i < historyCount; ++i)
        sumMs += frameTimes[i];
    
    avgFrameMs = (historyCount > 0) ? (sumMs / historyCount) : ms;
    avgFps = (historyCount > 0) ? (1000.0f / avgFrameMs) : fps;
}


void UserInterface::beginFrame()
{   
    if (!initialized) return;
    // Начинаем новый ImGui кадр
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UserInterface::buildUI(const AppState& state, AppCommands& commands)
{
    if (!initialized) return;

    ImGui::Begin("Simulation");

    ImGui::Text("Frame:  %.3f ms", avgFrameMs);
    ImGui::Text("FPS:    %.1f", avgFps);
    ImGui::Text("Sim dt: %.4f s", simDt);
    ImGui::Separator();

    bool pausedLocal = state.paused;
    if (ImGui::Checkbox("Paused", &pausedLocal))
    {
        commands.hasSetPaused = true;
        commands.setPausedValue = pausedLocal;
    }

    if (ImGui::Button("Step once")) commands.stepOnce = true;
    ImGui::SameLine();
    if (ImGui::Button("Reset (R)")) commands.reset = true;

    ImGui::End();
}

void UserInterface::endFrame()
{
    if (!initialized) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
