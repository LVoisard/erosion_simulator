#include "height_map/height_map.h"
#include "mesh/terrain_mesh.h"
#include "window/window.h"
#include "shader/shader.h"
#include "camera/camera.h"
#include "skybox/skybox.h"
#include <glm/glm.hpp>
#include "texture/texture.h"
#include <string>
#include <vector>
#include <chrono>

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

bool firstFrame = true;

struct FlowFlux
{
    double left;
    double right;
    double top;
    double bottom;
};

struct ErosionModel 
{
    double** terrainHeight; // b
    double** waterHeight; // d
    double** suspendedSedimentAmount; // s
    FlowFlux** outflowFlux; // f
    glm::vec2** velocity; // v
};

float waterflowRate = 0.1f;

int main()
{
    Window window(SCR_WIDTH, SCR_HEIGHT);
    Shader mainShader("shaders/main.vert", "shaders/main.frag");
    Shader waterShader("shaders/water.vert", "shaders/water.frag");
    Camera camera(&window, 5.0f, .25f, .5f);

    Texture dirtTexture("textures/dirt.jpg");
    Texture grassTexture("textures/grass.jpg");
    Texture sandTexture("textures/red_sand.jpg");
    Texture rockTexture("textures/rock.jpg");

    Texture redSandTexture("textures/red-clay-wall-albedo.png");
    // Texture meadowsGrassTexture("textures/patchy-meadow1_albedo.png");

    std::vector<std::string> skyboxFacesLocation;

    skyboxFacesLocation.push_back("textures/skybox/px.png");
    skyboxFacesLocation.push_back("textures/skybox/nx.png");
    skyboxFacesLocation.push_back("textures/skybox/py.png");
    skyboxFacesLocation.push_back("textures/skybox/ny.png");
    skyboxFacesLocation.push_back("textures/skybox/pz.png");
    skyboxFacesLocation.push_back("textures/skybox/nz.png");

    Skybox skybox(skyboxFacesLocation);

    // Max is 4096
    int mapSize = 1024;
    double minHeight = -100.0;
    double maxHeight = 516;
    double random = 3;
	
	HeightMap map(mapSize, minHeight, maxHeight, random);
	// map.saveHeightMapPPM("height_map.ppm");

	TerrainMesh terrainMesh(map.getheightMapSize(), &map);
    
    double** waterLevel = new double* [mapSize];

    ErosionModel model;
    model.terrainHeight = new double* [mapSize];
    model.waterHeight = new double* [mapSize];
    model.suspendedSedimentAmount = new double* [mapSize];
    model.outflowFlux = new FlowFlux * [mapSize];
    model.velocity = new glm::vec2* [mapSize];
    for (int i = 0; i < mapSize; i++) {
        model.terrainHeight[i] = new double [mapSize];
        model.waterHeight[i] = new double[mapSize];
        model.suspendedSedimentAmount[i] = new double[mapSize];
        model.outflowFlux[i] = new FlowFlux [mapSize];
        model.velocity[i] = new glm::vec2[mapSize];
        waterLevel[i] = new double[mapSize];
    }

    for (int y = 0; y < mapSize; y++)
        for (int x = 0; x < mapSize; x++)
        {
            model.terrainHeight[x][y] = map.samplePoint(x, y);
            model.waterHeight[x][y] = 0.25;
            model.suspendedSedimentAmount[x][y] = 0.0;
            model.outflowFlux[x][y] = FlowFlux{ 0.0, 0.0, 0.0, 0.0 };
            model.velocity[x][y] = glm::vec2(0.0);
            waterLevel[x][y] = model.terrainHeight[x][y] + model.waterHeight[x][y];
        }

    Mesh waterMesh;
    waterMesh.createSquareMesh(mapSize, waterLevel);

    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(90.0f), window.getAspectRatio(), 0.1f, 1000.0f);

    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldWindowClose())
    {
        auto newTime = std::chrono::high_resolution_clock::now();
        float deltaTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        if (window.getKeyDown(GLFW_KEY_R)) {
        //if (!firstFrame) {
            firstFrame = false;
            map.changeSeed();
            terrainMesh.updateMeshFromMap();
            // map.saveHeightMapPPM("height_map.ppm");
        }

        camera.update(deltaTime);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();

        skybox.DrawSkybox(view, proj);

        // draw our first triangle
        mainShader.use();
        mainShader.setMat4("model", model);
        mainShader.setMat4("view", view);
        mainShader.setMat4("projection", proj);

        mainShader.setTexture("texture0", 0);
        //meadowsGrassTexture.use(GL_TEXTURE0);
        sandTexture.use(GL_TEXTURE0);
        mainShader.setTexture("texture1", 1);
        redSandTexture.use(GL_TEXTURE1);
        mainShader.setTexture("texture2", 2);
        dirtTexture.use(GL_TEXTURE2);

        terrainMesh.draw();
        mainShader.stop();

        waterShader.use();
        waterShader.setMat4("model", model);
        waterShader.setMat4("view", view);
        waterShader.setMat4("projection", proj);

        waterMesh.draw();
        waterShader.stop();


        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        window.swapBuffers();
        window.updateInput();
        window.pollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}