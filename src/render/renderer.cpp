#include "renderer.h"
#include "common/config.h"

#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>


Renderer::Renderer(int width, int height, GLFWwindow* window):windowWidth(width), windowHeight(height), window(window) { }

static Renderer* s_instance = nullptr;

void Renderer::setWindow(GLFWwindow* wnd)
{
	window = wnd;
	s_instance = this;  
	if (window)
	{
		centerWindow();
	
		//glfwSetWindowUserPointer(window, this); // Пробрасываем указатель на экземпляр класса 
		glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

		initGL();
		initShaders();

		setMaxSpeed(2.5f); // Максимальная скорость для градиента цвета

		initGeometry();
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
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Достаём указатель на Renderer, который мы положили в user pointer
	//auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window)); // Нужно т.к. нельзя просто передать метод класса

	if (s_instance)
		s_instance->onResize(width, height);
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

	// Генерируем и настраиваем VAO и VBO для quad
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Описываем формат атрибута позиции (location = 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                      // location в шейдере
        3,                      // по 3 компоненты (x,y,z)
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),      // шаг между вершинами
        (void*)0                // смещение от начала
	);
	

	// instance: location=1 (vec4: x,y,z,radius), location=2 (float: speed)
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                      // location в шейдере
        4,                      // x, y, z, radius
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),      // шаг между вершинами
        (void*)0
	);
	glVertexAttribDivisor(1, 1);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,                      // location в шейдере
        1,                      // speed
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void*)(4 * sizeof(float))  // смещение после x,y,z,radius
	);
	glVertexAttribDivisor(2, 1);

	glBindVertexArray(0);
}

void Renderer::renderFrame(const Particles3D& particles)
{
	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);;

	int n = particles.count;
    if (n == 0)
        return;

	glUseProgram(shaderProgram);
	
	setCircleRadius(1.0f);

	// Собираем данные (x, y, z=0, radius, speed)
	std::vector<float> instanceData;
	instanceData.resize(n * 5);

	float radius = Config::particleRadius;
	for (int i = 0; i < n; ++i)
    {
		float vx = particles.vx[i];
		float vy = particles.vy[i];
		float speed = std::sqrt(vx * vx + vy * vy);

		instanceData[5 * i + 0] = particles.x[i];
		instanceData[5 * i + 1] = particles.y[i];
		instanceData[5 * i + 2] = particles.z[i];
		instanceData[5 * i + 3] = radius;
		instanceData[5 * i + 4] = speed;
	}

	// Переносим данные в InstanceVBO
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, 
				 instanceData.size() * sizeof(float),
				 instanceData.data(),
				 GL_STREAM_DRAW);

	// Рисуем
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, n);

}

void Renderer::setCircleRadius(float normalisedRadius)
{
	glUseProgram(shaderProgram);
	GLint loc = glGetUniformLocation(shaderProgram, "uRadius");
	if (loc != -1) glUniform1f(loc, normalisedRadius);
}

void Renderer::setMaxSpeed(float maxSpeed)
{
		glUseProgram(shaderProgram);
		GLint loc = glGetUniformLocation(shaderProgram, "uMaxSpeed");
		if (loc != -1)
			glUniform1f(loc, maxSpeed);
}

void Renderer::updateCamera(Camera3D& cam)
{
	// Синхронизируем aspect с текущим размером окна
    cam.setAspect(static_cast<float>(windowWidth) / static_cast<float>(windowHeight));

    glUseProgram(shaderProgram);

	glm::mat4 view = cam.getViewMatrix();
    GLint viewLoc = glGetUniformLocation(shaderProgram, "uView");
    if (viewLoc != -1)
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 proj = cam.getProjMatrix();
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProj");
    if (projLoc != -1)
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

	// Передаём оси камеры для billboard-рендеринга
	glm::vec3 right = cam.getRight();
	GLint rightLoc = glGetUniformLocation(shaderProgram, "uCameraRight");
	if (rightLoc != -1)
		glUniform3f(rightLoc, right.x, right.y, right.z);

	glm::vec3 camUp = cam.getCamUp();
	GLint camUpLoc = glGetUniformLocation(shaderProgram, "uCameraUp");
	if (camUpLoc != -1)
		glUniform3f(camUpLoc, camUp.x, camUp.y, camUp.z);
}

void Renderer::ensureInstanceBufferSize(int n) // Ресайзим VBO без перерегистрации (регистрация — в CUDA backend)
{
    if (n == lastInstanceCount) return;
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(n * 5 * sizeof(float)),
                 nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    lastInstanceCount = n;
}


void Renderer::renderFrameInterop(int particleCount) // Рендерим без загрузки данных — CUDA уже заполнила VBO
{
    glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);;

    if (particleCount <= 0) return;

    glUseProgram(shaderProgram);
    setCircleRadius(1.0f);

    glBindVertexArray(vao);
	
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particleCount);
    glBindVertexArray(0);
}
