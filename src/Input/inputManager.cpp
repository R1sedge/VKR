#include "inputManager.h"

void InputManager::update()
{
    if (!window) return;

    // Reset scroll delta each frame
    scrollDelta = 0.0;

    // Keyboard
    prev = curr;
    auto upd = [&](int key){
        curr[key] = (glfwGetKey(window, key) == GLFW_PRESS) ? 1 : 0;
    };

    upd(GLFW_KEY_SPACE);
    upd(GLFW_KEY_R);
    upd(GLFW_KEY_RIGHT);

    // Mouse buttons
    mousePrev = mouseCurr;
    auto updMouse = [&](int button){
        mouseCurr[button] = (glfwGetMouseButton(window, button) == GLFW_PRESS) ? 1 : 0;
    };
    updMouse(GLFW_MOUSE_BUTTON_LEFT);
    updMouse(GLFW_MOUSE_BUTTON_RIGHT);
    updMouse(GLFW_MOUSE_BUTTON_MIDDLE);

    // Mouse position
    prevMouseX = mouseX;
    prevMouseY = mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
}

bool InputManager::isMouseButtonDown(int button) const
{
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return mouseCurr[button] != 0;
}

bool InputManager::mouseButtonJustPressed(int button) const
{
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return mouseCurr[button] && !mousePrev[button];
}