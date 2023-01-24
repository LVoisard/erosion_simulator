#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"

class Window
{

public:
	Window(int width, int height);
	void init();
	void updateInput();

	GLFWwindow* getGlfwWindow() { return glfwWindow; }
	bool shouldWindowClose() { return glfwWindowShouldClose(glfwWindow); }
	void swapBuffers() { glfwSwapBuffers(glfwWindow); }
	void pollEvents() { glfwPollEvents(); }
	float getAspectRatio() { return (float)width / (float)height; }

	void setCursorMode(int cursorMode) { glfwSetInputMode(glfwWindow, GLFW_CURSOR, cursorMode); }

	bool getKey(int key) { return keys[key]; }
	bool getKeyDown(int key) { return keysDown[key]; }
	bool getKeyUp(int key) { return keysUp[key]; }

	double getMouseDeltaX() { return mouseDeltaX; }
	double getMouseDeltaY() { return mouseDeltaY; }

	double getMouseButton(int button) { return mouseButtons[button]; }
	double getMouseButtonDown(int button) { return mouseButtonsDown[button]; }
	double getMouseButtonUp(int button) { return mouseButtonsUp[button]; }

	double getMouseScrollY() { return mouseScrollY; }

private:
	int width;
	int height;

	int bufferWidth;
	int bufferHeight;
	GLFWwindow* glfwWindow;

	bool keys[1024] = { false };
	bool keysDown[1024] = { false };
	bool keysUp[1024] = { false };

	bool mouseButtons[8] = { false };
	bool mouseButtonsDown[8] = { false };
	bool mouseButtonsUp[8] = { false };

	double mouseScrollY = 0.0f;

	double mousePosX = 0;
	double mousePosY = 0;
	double mouseDeltaX = 0;
	double mouseDeltaY = 0;


	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void cursor_position_callback(GLFWwindow* window, double xPos, double yPos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

};

