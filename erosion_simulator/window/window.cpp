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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    glfwWindow = glfwCreateWindow(width, height, "Erosion Simulation", NULL, NULL);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    float heightScale, widthScale;
    glfwGetWindowContentScale(glfwWindow, &widthScale, &heightScale);
    io.FontGlobalScale = heightScale;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glViewport(0, 0, bufferWidth, bufferHeight - ImGui::GetFrameHeight());    

    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, framebuffer_size_callback);


}

void Window::updateInput()
{

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

float simSpeed = 1.f;
static bool showSimulationParameters = false;

void Window::Menu(ErosionModel* model)
{

    if (ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Simulation Parameters", NULL, &showSimulationParameters)) 
            {
                if (showSimulationParameters)
                {
                    width -= SIMULATION_PARAMETER_WINDOW_WIDTH;
                }
                else
                {
                    width += SIMULATION_PARAMETER_WINDOW_WIDTH;
                }
                
                glViewport(0, 0, width, height);
            }            
            ImGui::EndMenu();            
        }
        ImGui::EndMainMenuBar();
    }   

    if (showSimulationParameters) ShowSimulationParameters(model, &showSimulationParameters);
}

void Window::ShowSimulationParameters(ErosionModel* model, bool *open)
{
    if (ImGui::Begin("Simulation Parameters", open, ImGuiWindowFlags_NoCollapse + ImGuiWindowFlags_NoMove + ImGuiWindowFlags_NoResize))
    {
        // for setting the window in the right position / size
        ImGui::SetWindowSize(ImVec2(SIMULATION_PARAMETER_WINDOW_WIDTH, height));
        ImGui::SetWindowPos(ImVec2(width, ImGui::GetFrameHeight()));

        ImGui::Checkbox("Enable Simulation", &model->isModelRunning);
        ImGui::Checkbox("Enable Rain", &model->isRaining);
        ImGui::Checkbox("Enable Sediment Slippage", &model->useSedimentSlippage);

        ImGui::Spacing();

        ImGui::SliderInt("Simulation Speed", &model->simulationSpeed, 1, 10);
        ImGui::SliderInt("Rain Intensity", &model->rainIntensity, 1, 10);
        ImGui::SliderInt("Rain Amount", &model->rainAmount, 1, 10);

        ImGui::SliderFloat("Evaporation Rate", &model->evaporationRate, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Sea Level", &model->seaLevel, -100, 100, "%.0f");
        ImGui::SliderFloat("Slippage Angle", &model->slippageAngle, 0, 89, "%.0f");
        ImGui::SliderFloat("Sediment Capacity", &model->sedimentCapacity, 0.0f, 1.0f, "%.2f");


        ImGui::End();
    }
    
    if (!*open) {
        width += SIMULATION_PARAMETER_WINDOW_WIDTH;
        glViewport(0, 0, width, height);
    }

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    Window* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    currentWindow->width = showSimulationParameters ? width - SIMULATION_PARAMETER_WINDOW_WIDTH : width;
    currentWindow->height = height - ImGui::GetFrameHeight();
    currentWindow->bufferWidth = width;
    currentWindow->bufferHeight = height;
    
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, currentWindow->width, currentWindow->height);

}

