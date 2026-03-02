#include "inputManager.h"

void InputManager::update()
{
    if (!window) return ;
    prev = curr;
    
    // Лямюда функция
    auto upd = [&](int key){
        curr[key] = (glfwGetKey(window, key) == GLFW_PRESS) ? 1 : 0;
    };

    upd(GLFW_KEY_SPACE);
    upd(GLFW_KEY_R);
}