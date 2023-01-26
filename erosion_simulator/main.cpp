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
#include <mesh/water_mesh.h>

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

struct FlowFlux
{
    double left;
    double right;
    double top;
    double bottom;
};

struct ErosionCell
{
    double terrainHeight; // b
    double waterHeight; // d
    double suspendedSedimentAmount; // s
    FlowFlux outflowFlux; // f
    glm::vec2 velocity; // v
};

struct ErosionModel 
{
    int size;

    double** terrainHeights; // b
    double** waterHeights; // d
    double** suspendedSedimentAmounts; // s
    FlowFlux** outflowFlux; // f
    glm::vec2** velocities; // v

    ErosionCell getCell(int x, int y) {
        return ErosionCell{
            terrainHeights[x][y],
            waterHeights[x][y],
            suspendedSedimentAmounts[x][y],
            outflowFlux[x][y],
            velocities[x][y]
        };
    }

    std::vector<ErosionCell> getNeighbourCells(int x, int y) {
        std::vector<ErosionCell> cells;
        if (x > 0) {
            cells.push_back(
                ErosionCell{
                    terrainHeights[x - 1][y],
                    waterHeights[x - 1][y],
                    suspendedSedimentAmounts[x - 1][y],
                    outflowFlux[x - 1][y],
                    velocities[x - 1][y]
                });
        } 
        else if (x < size) {
            cells.push_back(
                ErosionCell{
                    terrainHeights[x + 1][y],
                    waterHeights[x + 1][y],
                    suspendedSedimentAmounts[x + 1][y],
                    outflowFlux[x + 1][y],
                    velocities[x + 1][y]
                });
        }

        if (y > 0) {
            cells.push_back(
                ErosionCell{
                    terrainHeights[x][y - 1],
                    waterHeights[x][y - 1],
                    suspendedSedimentAmounts[x][y - 1],
                    outflowFlux[x][y - 1],
                    velocities[x][y - 1]
                });
        }
        else if (y < size) {
            cells.push_back(
                ErosionCell{
                    terrainHeights[x][y + 1],
                    waterHeights[x][y + 1],
                    suspendedSedimentAmounts[x][y + 1],
                    outflowFlux[x][y + 1],
                    velocities[x][y + 1]
                });
        }

        return cells;
    }
};

float waterflowRate = 0.1f;

int main()
{
    Window window(SCR_WIDTH, SCR_HEIGHT);
    Shader mainShader("shaders/main.vert", "shaders/main.frag");
    Shader waterShader("shaders/water.vert", "shaders/water.frag");
    Camera camera(&window, 5.0f, .25f, .5f);

    Texture grassTexture("textures/grass.jpg");
    Texture sandTexture("textures/sand.jpg");
    Texture rockTexture("textures/rock.jpg");

    std::vector<std::string> skyboxFacesLocation;

    skyboxFacesLocation.push_back("textures/skybox/px.png");
    skyboxFacesLocation.push_back("textures/skybox/nx.png");
    skyboxFacesLocation.push_back("textures/skybox/py.png");
    skyboxFacesLocation.push_back("textures/skybox/ny.png");
    skyboxFacesLocation.push_back("textures/skybox/pz.png");
    skyboxFacesLocation.push_back("textures/skybox/nz.png");

    Skybox skybox(skyboxFacesLocation);

    // Max is 4096
    int mapSize = 512;
    double minHeight = -100.0;
    double maxHeight = 516;
    double random = 3;
	HeightMap map(mapSize, minHeight, maxHeight, random);
    HeightMap waterMap(mapSize, 0, 5, 5);

    ErosionModel model;

    model.terrainHeights = new double* [mapSize + 1];
    model.waterHeights = new double* [mapSize + 1];
    model.suspendedSedimentAmounts = new double* [mapSize + 1];
    model.outflowFlux = new FlowFlux * [mapSize + 1];
    model.velocities = new glm::vec2 * [mapSize + 1];

    for (int y = 0; y < mapSize + 1; y++)
    {
        model.terrainHeights[y] = new double [mapSize + 1];
        model.waterHeights[y] = new double [mapSize + 1];
        model.suspendedSedimentAmounts[y] = new double [mapSize + 1];
        model.outflowFlux[y] = new FlowFlux  [mapSize + 1];
        model.velocities[y] = new glm::vec2  [mapSize + 1];
    }

    for (int y = 0; y < mapSize + 1; y++)
    {
        for (int x = 0; x < mapSize + 1; x++)
        {
            model.terrainHeights[x][y] = map.samplePoint(x, y);
            model.waterHeights[x][y] = 0.0;
            model.suspendedSedimentAmounts[x][y] = 0.0;
            model.outflowFlux[x][y] = FlowFlux{};
            model.velocities[x][y] = glm::vec2(0.0f);
        }
    }

	TerrainMesh terrainMesh(map.getheightMapSize(), &map, mainShader);
    terrainMesh.init();
    WaterMesh waterMesh(map.getheightMapSize(), model.terrainHeights, model.waterHeights, waterShader);
    waterMesh.init();

    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(90.0f), window.getAspectRatio(), 0.1f, 1000.0f);

    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldWindowClose())
    {
        auto newTime = std::chrono::high_resolution_clock::now();
        float deltaTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // disable until update all meshes is done

        if (window.getKeyDown(GLFW_KEY_R)) {
            map.changeSeed();
            waterMap.changeSeed();
            terrainMesh.updateMeshFromMap(&map);
        }

        camera.update(deltaTime);

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
        grassTexture.use(GL_TEXTURE0);
        mainShader.setTexture("texture1", 1);
        sandTexture.use(GL_TEXTURE1);
        mainShader.setTexture("texture2", 2);
        rockTexture.use(GL_TEXTURE2);

        terrainMesh.draw();
        mainShader.stop();

        waterShader.use();
        waterShader.setMat4("model", model);
        waterShader.setMat4("view", view);
        waterShader.setMat4("projection", proj);

        waterMesh.draw();
        waterShader.stop();

        window.swapBuffers();
        window.updateInput();
        window.pollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}