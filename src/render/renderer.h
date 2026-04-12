#pragma once
#include <glad/glad.h> // до импорта glfw
#include <GLFW/glfw3.h>
#include <string>

#include "data/particleData.h"
#include "render/Camera3D.h" 

class Renderer
{
private:
	int windowWidth;
	int windowHeight;
	GLFWwindow* window;

	unsigned int shaderProgram = 0;
	unsigned int vbo = 0;
	unsigned int instanceVBO = 0;
	unsigned int vao = 0;

	int lastInstanceCount = 0;

public:
	Renderer(int width, int height, GLFWwindow* window);

	void setWindow(GLFWwindow* window);

	void renderFrame(const Particles2D& particles); // CPU rendering
	void renderFrameInterop(int particleCount); // Interop rendering

	void ensureInstanceBufferSize(int n);
	GLuint getInstanceVBO() const {return instanceVBO; }

	void updateCamera(Camera3D& cam);

	void setCircleRadius(float normalisedRadius);
	void setMaxSpeed(float maxSpeed);

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