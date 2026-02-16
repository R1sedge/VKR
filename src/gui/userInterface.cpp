#include "userInterface.h"

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

void UserInterface::render()
{
    if (!initialized)
        return;

    // Начинаем новый ImGui кадр
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug window");
    ImGui::Text("Hello, frow ImGui!");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}