#include "renderer.h"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>


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
	initShaders();
	initGeometry();
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

std::string Renderer::readShaderFile(const char* filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
		throw std::runtime_error(std::string("Cannot open shader file") + filePath);
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void Renderer::initShaders()
{
	std::string vertexSource = readShaderFile("shaders/basic.vert");
	std::string fragmentSource = readShaderFile("shaders/basic.frag");

	GLuint vertex = compileVertexShader(vertexSource);
	GLuint fragment = compileFragmentShader(fragmentSource);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertex);
	glAttachShader(shaderProgram, fragment);
	glLinkProgram(shaderProgram);
	 
	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
		throw std::runtime_error(infoLog);
	}

	glDeleteShader(vertex);
	glDeleteShader(fragment);

}

GLuint Renderer::compileVertexShader(const std::string vertexSource)
{
	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	const char* src = vertexSource.c_str();

	glShaderSource(vertex, 1, &src, nullptr);
	glCompileShader(vertex);

	GLint success;
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		throw std::runtime_error(infoLog);
	}
	return vertex;
}

GLuint Renderer::compileFragmentShader(const std::string fragmentSource)
{
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	const char* src = fragmentSource.c_str();

	glShaderSource(fragment, 1, &src, nullptr);
	glCompileShader(fragment);

	GLint success;
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		throw std::runtime_error(infoLog);
	}
	return fragment;
}

void Renderer::initGeometry()
{
	float vertices[] = {
        -0.5f, -0.5f, 0.0f,  // левая нижняя
         0.5f, -0.5f, 0.0f,  // правая нижняя
         0.0f,  0.5f, 0.0f   // верхняя
    };

	// Генерируем и настраиваем VAO и VBO
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Описываем формат атрибута позиции (location = 0)
	glVertexAttribPointer(
		0,                      // location в шейдере
        3,                      // по 3 компоненты (x,y,z)
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),      // шаг между вершинами
        (void*)0                // смещение от начала
	);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void Renderer::renderFrame()
{
	glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
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