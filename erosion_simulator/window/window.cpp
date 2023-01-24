#include "window.h"
#include <iostream>

Window::Window(int width, int height)
    :width(width), height(height)
{
    init();
}

void Window::init()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    glfwWindow = glfwCreateWindow(width, height, "LearnOpenGL", NULL, NULL);
    if (glfwWindow == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }

    glfwGetFramebufferSize(glfwWindow, &bufferWidth, &bufferHeight);
    glfwMakeContextCurrent(glfwWindow);

    glfwSetKeyCallback(glfwWindow, key_callback);
    glfwSetCursorPosCallback(glfwWindow, cursor_position_callback);
    glfwSetMouseButtonCallback(glfwWindow, mouse_button_callback);
    glfwSetScrollCallback(glfwWindow, scroll_callback);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, bufferWidth, bufferHeight);

    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, framebuffer_size_callback);
}

void Window::updateInput()
{
    if (keys[GLFW_KEY_ESCAPE])
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);


    for (int i = 0; i < 1024; i++) 
    {
        keysDown[i] = false;
        keysUp[i] = false;
    }

    for (int i = 0; i < 8; i++)
    {
        mouseButtonsDown[i] = false;
        mouseButtonsUp[i] = false;
    }

    mouseDeltaX = 0;
    mouseDeltaY = 0;
    mouseScrollY = 0;
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Window* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        currentWindow->keysDown[key] = true;
        currentWindow->keys[key] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        currentWindow->keysUp[key] = true;
        currentWindow->keys[key] = false;
    }
}

bool firstTime = true;
void Window::cursor_position_callback(GLFWwindow* window, double xPos, double yPos)
{
    Window* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    
    if (firstTime) {
        currentWindow->mousePosX = xPos;
        currentWindow->mousePosY = yPos;
        firstTime = false;
        return;
    }

    currentWindow->mouseDeltaX = xPos - currentWindow->mousePosX;
    currentWindow->mouseDeltaY = currentWindow->mousePosY - yPos;
    currentWindow->mousePosX = xPos;
    currentWindow->mousePosY = yPos;
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Window* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        currentWindow->mouseButtons[button] = true;
        currentWindow->mouseButtonsDown[button] = true;
    } 
    else if (action == GLFW_RELEASE)
    {
        currentWindow->mouseButtons[button] = false;
        currentWindow->mouseButtonsUp[button] = true;
    }
}

void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Window* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

    currentWindow->mouseScrollY = yoffset;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
