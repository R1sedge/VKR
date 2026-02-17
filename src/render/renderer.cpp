#include "renderer.h"
#include "common/config.h"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>


Renderer::Renderer(int width, int height, GLFWwindow* window):windowWidth(width), windowHeight(height), window(window)
{
	
}	

void Renderer::setWindow(GLFWwindow* wnd)
{
	window = wnd;
	if (window)
	{
		centerWindow();
	
		glfwSetWindowUserPointer(window, this); // Пробрасываем указатель на экземпляр класса 
		glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

		initGL();
		initShaders();

		updateProjection();

		initGeometry();
		setTriangleColor(0.2f, 0.4f, 0.8f, 1.0f);
	}
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

	updateProjection();

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
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
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
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,

    -1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f
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

void Renderer::renderFrame(const Particles2D& particles)
{
	glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(vao);

	setTriangleColor(0.2f, 0.4f, 0.8f, 1.0f);
	setCircleRadius(1.0f);

	float radius = Config::particleRadius;

	for (int i = 0; i < particles.count; ++i)
    {
		float x = particles.x[i];
        float y = particles.y[i];
		
		setModelMatrix(x, y, radius);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
}

void Renderer::setTriangleColor(float r, float g, float b, float a)
{
	glUseProgram(shaderProgram);
	GLint loc = glGetUniformLocation(shaderProgram, "uColor"); // Поиск переменной по имени
	if (loc != -1) glUniform4f(loc, r, g, b, a);
}

void Renderer::setCircleRadius(float normalisedRadius)
{
	glUseProgram(shaderProgram);
	GLint loc = glGetUniformLocation(shaderProgram, "uRadius");
	if (loc != -1) glUniform1f(loc, normalisedRadius);
}

void Renderer::setModelMatrix(float x, float y, float radius)
{
	glUseProgram(shaderProgram);
	float s = radius;

	float model[16] = {
        s, 0, 0, 0,   // scale x
        0, s, 0, 0,   // scale y
        0, 0, 1, 0,   // колонка 2
        x, y, 0, 1  // колонка 3 (translation)
    };

	GLint loc = glGetUniformLocation(shaderProgram, "uModel");
	if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, model);
}

void Renderer::setOrthoProjection(float left, float right, float bottom, float top)
{
	glUseProgram(shaderProgram);
	float proj[16] = {
        2.0f / (right - left),  		 0,                                0, 0,
        0,                      		 2.0f / (top - bottom),            0, 0,
        0,                      		 0,                               -1, 0,
       -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0, 1
    };

	GLint loc = glGetUniformLocation(shaderProgram, "uProjection");
	if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, proj);
}

void Renderer::updateProjection()
{
	float halfWorldW = 	windowWidth / (2 * Config::pixelsPerUnits);
	float halfWorldH = 	windowHeight / (2 * Config::pixelsPerUnits);

	setOrthoProjection(-halfWorldW, +halfWorldW, -halfWorldH, +halfWorldH);
}