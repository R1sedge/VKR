#pragma once
#include <glad/glad.h> // Это перед glfw
#include <GLFW/glfw3.h>


class Renderer
{
private:
	int windowWidth;
	int windowHeight;
	GLFWwindow* window;

public:
	Renderer(int width, int height);

	void mainLoop();
	void renderFrame();

private:
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	void onResize(int width, int height);

	void centerWindow();
	void initGL();
	void initShaders();
	void initGeometry();
};