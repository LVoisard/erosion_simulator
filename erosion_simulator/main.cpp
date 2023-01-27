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

#include <iostream>

#include <random>

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

std::default_random_engine gen;
std::uniform_int_distribution<> distr(0, 512);

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

	ErosionCell* getCell(int x, int y) {
		ErosionCell cell{
			terrainHeights[x][y],
			waterHeights[x][y],
			suspendedSedimentAmounts[x][y],
			outflowFlux[x][y],
			velocities[x][y]
		};
		return &cell;
	}

	ErosionCell* getNeighbourLeft(int x, int y) {
		ErosionCell cell;
		if (x > 0) {
			cell = ErosionCell{
				terrainHeights[x - 1][y],
				waterHeights[x - 1][y],
				suspendedSedimentAmounts[x - 1][y],
				outflowFlux[x - 1][y],
				velocities[x - 1][y]
			};
		}
		else
			return nullptr;
		return &cell;
	}

	ErosionCell* getNeighbourRight(int x, int y) {
		//right
		ErosionCell cell;
		if (x < size) {
			cell = ErosionCell{
				terrainHeights[x + 1][y],
				waterHeights[x + 1][y],
				suspendedSedimentAmounts[x + 1][y],
				outflowFlux[x + 1][y],
				velocities[x + 1][y]
			};
		}
		else
			return nullptr;
		return &cell;
	}

	ErosionCell* getNeighbourTop(int x, int y) {
		//right
		ErosionCell cell;
		if (y < size) {
			cell = ErosionCell{
				terrainHeights[x][y + 1],
				waterHeights[x][y + 1],
				suspendedSedimentAmounts[x][y + 1],
				outflowFlux[x][y + 1],
				velocities[x][y + 1]
			};
		}
		else
			return nullptr;
		return &cell;
	}

	ErosionCell* getNeighbourBottom(int x, int y) {
		//right
		ErosionCell cell;
		if (y > 0) {
			cell = ErosionCell{
				terrainHeights[x][y - 1],
				waterHeights[x][y - 1],
				suspendedSedimentAmounts[x][y - 1],
				outflowFlux[x][y - 1],
				velocities[x][y - 1]
			};
		}
		else
			return nullptr;
		return &cell;
	}
};

float waterflowRate = 0.1f;

Window window(SCR_WIDTH, SCR_HEIGHT);
Shader mainShader("shaders/main.vert", "shaders/main.frag");
Shader waterShader("shaders/water.vert", "shaders/water.frag");
Camera camera(&window, 5.0f, .25f, .5f);

Texture grassTexture("textures/grass.jpg");
Texture sandTexture("textures/sand.jpg");
Texture rockTexture("textures/rock.jpg");

std::vector<std::string> skyboxFacesLocation{
	"textures/skybox/px.png",
	"textures/skybox/nx.png",
	"textures/skybox/py.png",
	"textures/skybox/ny.png",
	"textures/skybox/pz.png",
	"textures/skybox/nz.png"
};

Skybox skybox(skyboxFacesLocation);
ErosionModel model;

// Max is 4096

// max size for cpu computations is 512,
// 256 is better
int mapSize = 256;
double minHeight = 0.0;
double maxHeight = 1;
double random = 0;
HeightMap map(mapSize, minHeight, maxHeight, random);

TerrainMesh* terrainMesh;
WaterMesh* waterMesh;


void initModel()
{
	model.terrainHeights = new double* [mapSize + 1];
	model.waterHeights = new double* [mapSize + 1];
	model.suspendedSedimentAmounts = new double* [mapSize + 1];
	model.outflowFlux = new FlowFlux * [mapSize + 1];
	model.velocities = new glm::vec2 * [mapSize + 1];

	for (int y = 0; y < mapSize + 1; y++)
	{
		model.terrainHeights[y] = new double[mapSize + 1];
		model.waterHeights[y] = new double[mapSize + 1];
		model.suspendedSedimentAmounts[y] = new double[mapSize + 1];
		model.outflowFlux[y] = new FlowFlux[mapSize + 1];
		model.velocities[y] = new glm::vec2[mapSize + 1];
	}

	for (int y = 0; y < mapSize + 1; y++)
	{
		for (int x = 0; x < mapSize + 1; x++)
		{
			model.terrainHeights[x][y] = map.samplePoint(x, y);
			model.waterHeights[x][y] = 5.0;
			model.suspendedSedimentAmounts[x][y] = 0.0;
			model.outflowFlux[x][y] = FlowFlux{};
			model.velocities[x][y] = glm::vec2(0.0f);
		}
	}
}

void resetModel()
{
	for (int y = 0; y < mapSize + 1; y++)
	{
		for (int x = 0; x < mapSize + 1; x++)
		{
			model.terrainHeights[x][y] = map.samplePoint(x, y);
			model.waterHeights[x][y] = 5.0;
			model.suspendedSedimentAmounts[x][y] = 0.0;
			model.outflowFlux[x][y] = FlowFlux{};
			model.velocities[x][y] = glm::vec2(0.0f);
		}
	}

	terrainMesh->updateMeshFromHeights(&model.terrainHeights);
	waterMesh->updateMeshFromHeights(&model.terrainHeights, &model.waterHeights);
}

void updateModel(float dt)
{
	// each frame, water should uniformly increment accross the grid

	for (int y = 0; y < mapSize + 1; y++)
	{
		for (int x = 0; x < mapSize + 1; x++)
		{
			if (distr(gen) == 0)
				model.waterHeights[x][y] += dt * 10;
		}
	}

	// then calculate the outflow of water to other cells

	for (int y = 0; y < mapSize + 1; y++)
	{
		for (int x = 0; x < mapSize + 1; x++)
		{
			// left output
			double area = 1; // area of cell
			double l = 1; // length of cell
			double g = 9.8;
			double b = model.getCell(x, y)->terrainHeight;
			double d = model.getCell(x, y)->waterHeight;

			

			if (model.getNeighbourLeft(x, y) != nullptr) {

				double dh =
					b +
					d -
					model.getNeighbourLeft(x, y)->terrainHeight -
					model.getNeighbourLeft(x, y)->waterHeight;
				double f = std::max(0.0, model.outflowFlux[x][y].left + dt * area * ((g * dh) / l));
				model.outflowFlux[x][y].left = f;
			}
			else {
				model.outflowFlux[x][y].left = 0.0;
			}

			if (model.getNeighbourRight(x, y) != nullptr) {

				double dh =
					b +
					d -
					model.getNeighbourRight(x, y)->terrainHeight -
					model.getNeighbourRight(x, y)->waterHeight;
				double f = std::max(0.0, model.outflowFlux[x][y].right + dt * area * ((g * dh) / l));
				model.outflowFlux[x][y].right = f;
			}
			else {
				model.outflowFlux[x][y].right = 0.0;
			}

			if (model.getNeighbourTop(x, y) != nullptr) {

				double dh =
					b +
					d -
					model.getNeighbourTop(x, y)->terrainHeight -
					model.getNeighbourTop(x, y)->waterHeight;
				double f = std::max(0.0, model.outflowFlux[x][y].top + dt * area * ((g * dh) / l));
				model.outflowFlux[x][y].top = f;
			}
			else {
				model.outflowFlux[x][y].top = 0.0;
			}

			if (model.getNeighbourBottom(x, y) != nullptr) {

				double dh =
					b +
					d -
					model.getNeighbourBottom(x, y)->terrainHeight -
					model.getNeighbourBottom(x, y)->waterHeight;
				double f = std::max(0.0, model.outflowFlux[x][y].bottom + dt * area * ((g * dh) / l));
				model.outflowFlux[x][y].bottom = f;
			}
			else {
				model.outflowFlux[x][y].bottom = 0.0;
			}

			double p1 = model.getCell(x, y)->waterHeight * l * l;
			double p2 = model.getCell(x, y)->outflowFlux.left +
				model.getCell(x, y)->outflowFlux.right +
				model.getCell(x, y)->outflowFlux.top +
				model.getCell(x, y)->outflowFlux.bottom;

			double K = std::min(1.0, p1 / (dt * p2));

			model.outflowFlux[x][y].left *= K;
			model.outflowFlux[x][y].right *= K;
			model.outflowFlux[x][y].top *= K;
			model.outflowFlux[x][y].bottom *= K;
		}
	}

	// recieve water from neighbors and send out to neighbors

	for (int y = 0; y < mapSize + 1; y++)
	{
		for (int x = 0; x < mapSize + 1; x++)
		{
			FlowFlux current = model.getCell(x, y)->outflowFlux;

			double fin = 0.0;
			if (model.getNeighbourLeft(x, y) != nullptr)
				fin += model.getNeighbourLeft(x, y)->outflowFlux.right;
			if (model.getNeighbourRight(x, y) != nullptr)
				fin += model.getNeighbourRight(x, y)->outflowFlux.left;
			if (model.getNeighbourTop(x, y) != nullptr)
				fin += model.getNeighbourTop(x, y)->outflowFlux.bottom;
			if (model.getNeighbourBottom(x, y) != nullptr)
				fin += model.getNeighbourBottom(x, y)->outflowFlux.top;

			double fout = current.left + current.right + current.top + current.bottom;

			double dV = dt * (fin - fout);
			model.waterHeights[x][y] = model.waterHeights[x][y] + dV / 1;


		}
	}

	terrainMesh->updateMeshFromHeights(&model.terrainHeights);
	waterMesh->updateMeshFromHeights(&model.terrainHeights, &model.waterHeights);
}

bool isRaining = false;

int main()
{
	initModel();

	terrainMesh = new TerrainMesh(map.getheightMapSize(), &model.terrainHeights, mainShader);
	waterMesh = new WaterMesh(map.getheightMapSize(), &model.terrainHeights, &model.waterHeights, waterShader);

	terrainMesh->init();
	waterMesh->init();

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
			resetModel();
		}

		if (window.getKeyDown(GLFW_KEY_ENTER) && !isRaining) {
			isRaining = true;
		}

		if (isRaining)
		{
			updateModel(deltaTime);
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

		terrainMesh->draw();
		mainShader.stop();

		waterShader.use();
		waterShader.setMat4("model", model);
		waterShader.setMat4("view", view);
		waterShader.setMat4("projection", proj);

		waterMesh->draw();
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