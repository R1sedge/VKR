#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class UserInterface
{
public:
    UserInterface() = default;
    ~UserInterface();

    bool initialize(GLFWwindow* window);

    void render();

private:
    bool initialized = false;
};