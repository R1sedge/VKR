#pragma once
#include <glad/glad.h> // ��� ����� glfw
#include <GLFW/glfw3.h>
#include <string>


class Renderer
{
private:
	int windowWidth;
	int windowHeight;
	GLFWwindow* window;

	unsigned int shaderProgram = 0;
	unsigned int vbo = 0;
	unsigned int vao = 0;

public:
	Renderer(int width, int height);

	void mainLoop();
	void renderFrame();

private:
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	void onResize(int width, int height);

	void centerWindow();
	void initGL();

	std::string readShaderFile(const char* filePath);
	GLuint compileVertexShader(const std::string vertexSource);
	GLuint compileFragmentShader(const std::string fragmentSource);
	void initShaders();
	void initGeometry();
};