#pragma once
#include <array>
#include <GLFW/glfw3.h>

class InputManager
{
public:
    void setWindow(GLFWwindow* wnd) { window = wnd; }

    void update();
    bool justPressed(int key) const { return curr[key] && !prev[key]; }

    // Mouse buttons
    bool isMouseButtonDown(int button) const;
    bool mouseButtonJustPressed(int button) const;

    // Mouse position (screen coordinates)
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }

    double getDeltaX() const { return mouseX - prevMouseX; }
    double getDeltaY() const { return mouseY - prevMouseY; }

    // Mouse scroll
    double getScrollDelta() const { return scrollDelta; }
    void addScrollDelta(double delta) { scrollDelta += delta; }
    void resetScrollDelta() { scrollDelta = 0.0; }

private:
    GLFWwindow* window = nullptr;

    std::array<uint8_t, GLFW_KEY_LAST + 1> curr{};
    std::array<uint8_t, GLFW_KEY_LAST + 1> prev{};

    // Mouse state
    std::array<uint8_t, GLFW_MOUSE_BUTTON_LAST + 1> mouseCurr{};
    std::array<uint8_t, GLFW_MOUSE_BUTTON_LAST + 1> mousePrev{};
    
    double mouseX = 0.0;
    double mouseY = 0.0;
    double prevMouseX = 0.0;
    double prevMouseY = 0.0;

    double scrollDelta = 0.0;
};