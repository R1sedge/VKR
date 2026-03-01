#pragma once
#include <array>
#include <GLFW/glfw3.h>

class InputManager
{
public:
    void setWindow(GLFWwindow* wnd) { window = wnd; }

    void update();
    bool justPressed(int key) const { return curr[key] && !prev[key]; }

private:
    GLFWwindow* window;

    std::array<uint8_t, GLFW_KEY_LAST + 1> curr{};
    std::array<uint8_t, GLFW_KEY_LAST + 1> prev{};
};