#include "window.h"

#include <iostream>
#include <string>

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


void Window::Menu(ErosionModel* model, SimulationParametersUI* params)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Save Height Map", NULL, &showSaveMenu);
            if(params->showRegenButton)
            if(ImGui::MenuItem("Regenerate Heightmap", NULL))
            {
                params->regenerateHeightMapRequested = true;
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Simulation Parameters", NULL, &showSimulationParameters);
            ImGui::MenuItem("Paint Brush Settings", NULL, &showPaintBrushMenu);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) 
        {
            if (ImGui::BeginMenu("Terrain Debug Modes"))
            {
                if(ImGui::MenuItem("Normal", NULL))
                {
                    model->terrainDebugMode = TerrainDebugMode::TERRAIN_NORMAL;
                }
                if (ImGui::MenuItem("Erosion Susceptibility", NULL))
                {
                    model->terrainDebugMode = TerrainDebugMode::TERRAIN_SUSCEPTIBILITY;
                }
                if (ImGui::MenuItem("Sediment Deposition", NULL))
                {
                    model->terrainDebugMode = TerrainDebugMode::TERRAIN_SEDIMENT_DEPOSITION;
                }
                if (ImGui::MenuItem("Invisible", NULL))
                {
                    model->terrainDebugMode = TerrainDebugMode::TERRAIN_INVISIBLE;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Water Debug Modes"))
            {
                if (ImGui::MenuItem("Normal", NULL))
                {
                    model->waterDebugMode = WaterDebugMode::WATER_NORMAL;
                }
                if (ImGui::MenuItem("Velocity", NULL))
                {
                    model->waterDebugMode = WaterDebugMode::WATER_VELOCITY;
                }
                if (ImGui::MenuItem("Sediment Transport", NULL))
                {
                    model->waterDebugMode = WaterDebugMode::WATER_SEDIMENT_TRANSPORT;
                }
                if (ImGui::MenuItem("Invisible", NULL))
                {
                    model->waterDebugMode = WaterDebugMode::WATER_INVISIBLE;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }   

    if (showSimulationParameters) ShowSimulationParameters(model, params, &showSimulationParameters);
    if (showPaintBrushMenu) ShowPaintBrushMenu(model, params, &showPaintBrushMenu);
    if (showSaveMenu) ShowSaveMenu(params, &showSaveMenu);
}

void Window::ShowSimulationParameters(ErosionModel* model, SimulationParametersUI* params, bool *open)
{
    if (ImGui::Begin("Simulation Parameters", open))
    {
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

}

void Window::ShowPaintBrushMenu(ErosionModel* model, SimulationParametersUI* params, bool* open)
{
    static std::string currentBrush = "Water Add";
    if (ImGui::Begin("Paint Brush Settings", open))
    {
        ImGui::Checkbox("Enable Paint Brush", &model->castRays);

        ImGui::SliderFloat("Brush Size", &model->brushRadius, 1.f, 50.0f);
        ImGui::SliderFloat("Brush Intensity", &model->brushIntensity, 1.f, 50.0f);

        ImGui::Separator();

        ImGui::Text(std::string("Current Brush: " + currentBrush).c_str());

        if (ImGui::Button("Water Add"))
        {
            model->paintMode = PaintMode::WATER_ADD;
            currentBrush = "Water Add";
        }
        if (ImGui::Button("Water Remove"))
        {
            model->paintMode = PaintMode::WATER_REMOVE;
            currentBrush = "Water Remove";
        }
        if (ImGui::Button("Water Source"))
        {
            model->paintMode = PaintMode::WATER_SOURCE;
            currentBrush = "Water Source";
        }
        if (ImGui::Button("Terrain Add"))
        {
            model->paintMode = PaintMode::TERRAIN_ADD;
            currentBrush = "Terrain Add";
        }
        if (ImGui::Button("Terrain Remove"))
        {
            model->paintMode = PaintMode::TERRAIN_REMOVE;
            currentBrush = "Terrain Remove";
        }

        ImGui::Separator();

        if(ImGui::Button("Remove All Sources"))
        {
            model->waterSources.clear();
        }
        

        ImGui::End();
    }
}

void Window::ShowSaveMenu(SimulationParametersUI* params, bool* open)
{
    if (ImGui::Begin("Save Heightmap", open))
    {
        ImGui::InputText("File Name", params->fileSaveName, 100);
        if (ImGui::Button("Save"))
        {
            params->saveHeightMapRequested = true;
            *open = false;
        }
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    Window* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    currentWindow->width = width;
    currentWindow->height = height - ImGui::GetFrameHeight();
    currentWindow->bufferWidth = width;
    currentWindow->bufferHeight = height;
    
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, currentWindow->width, currentWindow->height);

}

