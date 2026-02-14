#include "renderer.h"
#include <stdexcept>

Renderer::Renderer(int width, int height):windowWidth(width), windowHeight(height)
{
	if (!glfwInit())
		throw std::runtime_error("GLFW init failed");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(windowWidth, windowHeight, "Simulation", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		throw std::runtime_error("Window creation failed");
	}

	centerWindow();
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // v-sync

	if (!gladLoadGL()) 
		throw std::runtime_error("GLAD init failed");

	glfwSetWindowUserPointer(window, this); // Пробрасываем указатель на экземпляр класса 
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	initGL();
	//initShaders();
	//initGeometry();
}	

void Renderer::centerWindow()
{
	// Центрирование окна
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	if (primaryMonitor)
	{
		const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
		if (videoMode)
		{
			int monitorX, monitorY;
			glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY); // Позиция левого верхнего края экрана

			int windowPosX = monitorX + (videoMode->width - windowWidth) / 2;
			int windowPosY = monitorY + (videoMode->height - windowHeight) / 2;

			glfwSetWindowPos(window, windowPosX, windowPosY);
		}
	}
}

void Renderer::initGL()
{
	glViewport(0, 0, windowWidth, windowHeight);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Достаём указатель на Renderer, который мы положили в user pointer
	auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window)); // Нужно т.к. нельзя просто передать метод класса
	if (renderer)
		renderer->onResize(width, height);
}

void Renderer::onResize(int width, int height) 
{
	// Обновляем хранимый размер окна.
	windowWidth = width;
	windowHeight = height;

	// Настраиваем viewport под новый размер буфера кадра
	glViewport(0, 0, width, height);
}

void Renderer::renderFrame()
{
	glClearColor(0.5f, 0.5f, 0.9f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::mainLoop()
{
	while(!glfwWindowShouldClose(window))
	{
		renderFrame();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}