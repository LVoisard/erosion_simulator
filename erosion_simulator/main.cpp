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

#define GLM_FORCE_RADIANS

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

struct FlowFlux
{
	float left;
	float right;
	float top;
	float bottom;

	FlowFlux() {
		left = 0.0f;
		right = 0.0f;
		top = 0.0f;
		bottom = 0.0f;
	}

	float getTotal() {
		return left + right + top + bottom;
	}
};

struct ErosionCell
{
	float terrainHeight; // b
	float waterHeight; // d
	float suspendedSedimentAmount; // s
	FlowFlux outflowFlux; // f
	glm::vec2 velocity; // v
	float terrainHardness;
};

struct ErosionModel
{
	int size;

	float** terrainHeights; // b
	float** waterHeights; // d
	float** suspendedSedimentAmounts; // s
	FlowFlux** outflowFlux; // f
	glm::vec2** velocities; // v
	float** terrainHardness;

	ErosionModel(int size) {
		this->size = size;
		this->terrainHeights = new float* [this->size];
		this->waterHeights = new float* [this->size];
		this->suspendedSedimentAmounts = new float* [this->size];
		this->outflowFlux = new FlowFlux * [this->size];
		this->velocities = new glm::vec2 * [this->size];
		this->terrainHardness = new float* [this->size];

		for (int i = 0; i < size; i++)
		{
			this->terrainHeights[i] = new float[this->size];
			this->waterHeights[i] = new float[this->size];
			this->suspendedSedimentAmounts[i] = new float[this->size];
			this->outflowFlux[i] = new FlowFlux[this->size];
			this->velocities[i] = new glm::vec2[this->size];
			this->terrainHardness[i] = new float[this->size];

		}
	}

	ErosionCell* getCell(int x, int y) {
		if (x < 0 || x >= size || y < 0 || y >= size)
			return nullptr;

		ErosionCell cell{
			terrainHeights[x][y],
			waterHeights[x][y],
			suspendedSedimentAmounts[x][y],
			outflowFlux[x][y],
			velocities[x][y],
			terrainHardness[x][y],
		};
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

// Max is 4096

// max size for cpu computations is 512,
// 256 is better
//2 ^ n
int mapSize = 256;
float minHeight = 0.0f;
float maxHeight = 100;
float random = 3;
HeightMap map(mapSize, minHeight, maxHeight, random);

TerrainMesh* terrainMesh;
WaterMesh* waterMesh;

ErosionModel model(mapSize + 1);

std::default_random_engine gen;
std::uniform_int_distribution<> distr(0, mapSize* mapSize);

bool isRaining = false;
bool isModelRunning = false;

void initModel()
{
	for (int y = 0; y < model.size; y++)
	{
		for (int x = 0; x < model.size; x++)
		{
			model.terrainHeights[x][y] = map.samplePoint(x, y);
			model.waterHeights[x][y] = 5.0f;
			model.suspendedSedimentAmounts[x][y] = 0.0f;
			model.outflowFlux[x][y] = FlowFlux{};
			model.velocities[x][y] = glm::vec2(0.0f);
			model.terrainHardness[x][y] = 0.1f;
		}
	}
}

void resetModel()
{
	for (int y = 0; y < model.size; y++)
	{
		for (int x = 0; x < model.size; x++)
		{
			model.terrainHeights[x][y] = map.samplePoint(x, y);
			model.waterHeights[x][y] = 5.0f;
			model.suspendedSedimentAmounts[x][y] = 0.0f;
			model.outflowFlux[x][y] = FlowFlux{};
			model.velocities[x][y] = glm::vec2(0.0f);
			model.terrainHardness[x][y] = 0.1f;
		}
	}

	terrainMesh->updateOriginalHeights(&model.terrainHeights);
	terrainMesh->updateMeshFromHeights(&model.terrainHeights);
	waterMesh->updateMeshFromHeights(&model.terrainHeights, &model.waterHeights, &model.velocities);
}
float simulationSpeed = 1.0f;

float rainIntensity = 1.0f;
float rainAmount = 1.0f;
float evaporationRate = 0.015f;

float fluidDensity = 1.0f;
// dont know if there should be more
float gravitationalAcc = 9.807f;
float length = 1.0f;
float area = length * length;

float sedimentCapacity = 0.01f;

void addWater(float dt) {
	if (isRaining) {
		for (int y = 0; y < model.size; y++)
		{
			for (int x = 0; x < model.size; x++)
			{
				if (distr(gen) <= rainAmount * mapSize)
					model.waterHeights[x][y] += dt * rainIntensity * simulationSpeed;
			}
		}
	}
}
void calculateModelOutflowFlux(float dt)
{
	for (int y = 0; y < model.size; y++)
	{
		for (int x = 0; x < model.size; x++)
		{
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					if (abs(i) == abs(j)) continue;
					float f = 0.0f;
					if (model.getCell(x + i, y + j) != nullptr) {
						float dHeight = model.terrainHeights[x][y] + model.waterHeights[x][y] - (model.terrainHeights[x + i][y + j] + model.waterHeights[x + i][y + j]);
						float dPressure = fluidDensity * gravitationalAcc * dHeight;
						float acceleration = dPressure / (fluidDensity * length);
						f = dt * simulationSpeed * area * acceleration;
					}

					// compute flux
					if (j == -1) {
						model.outflowFlux[x][y].bottom = std::max(0.0f, model.outflowFlux[x][y].bottom + f);
					}
					else if (j == 1) {
						model.outflowFlux[x][y].top = std::max(0.0f, model.outflowFlux[x][y].top + f);
					}
					else if (i == -1) {
						model.outflowFlux[x][y].left = std::max(0.0f, model.outflowFlux[x][y].left + f);
					}
					else if (i == 1) {
						model.outflowFlux[x][y].right = std::max(0.0f, model.outflowFlux[x][y].right + f);
					}

					// rescale
					if (j == -1) {
						model.outflowFlux[x][y].bottom *= std::min(1.0f, model.waterHeights[x][y] * length * length / (model.outflowFlux[x][y].bottom * dt));
					}
					else if (j == 1) {
						model.outflowFlux[x][y].top *= std::min(1.0f, model.waterHeights[x][y] * length * length / (model.outflowFlux[x][y].top * dt));
					}
					else if (i == -1) {
						model.outflowFlux[x][y].left *= std::min(1.0f, model.waterHeights[x][y] * length * length / (model.outflowFlux[x][y].left * dt));
					}
					else if (i == 1) {
						model.outflowFlux[x][y].right *= std::min(1.0f, model.waterHeights[x][y] * length * length / (model.outflowFlux[x][y].right * dt));
					}
				}
			}

			model.outflowFlux[x][y].bottom = std::max(0.0f, model.outflowFlux[x][y].bottom);
			model.outflowFlux[x][y].top = std::max(0.0f, model.outflowFlux[x][y].top);
			model.outflowFlux[x][y].left = std::max(0.0f, model.outflowFlux[x][y].left);
			model.outflowFlux[x][y].right = std::max(0.0f, model.outflowFlux[x][y].right);
		}
	}
}
void calculateModelWaterHeights(float dt)
{
	for (int y = 0; y < model.size; y++)
	{
		for (int x = 0; x < model.size; x++)
		{
			float finX = 0.0f;
			float finY = 0.0f;
			float foutX = model.outflowFlux[x][y].left + model.outflowFlux[x][y].right;
			float foutY = model.outflowFlux[x][y].top + model.outflowFlux[x][y].bottom;

			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					if (abs(i) == abs(j)) continue;
					if (model.getCell(x - i, y - j) != nullptr) {
						FlowFlux flux = model.outflowFlux[x - i][y - j];
						if (j == -1) {
							finY += flux.bottom;
						}
						else if (j == 1) {
							finY += flux.top;
						}
						else if (i == -1) {
							finX += flux.left;
						}
						else if (i == 1) {
							finX += flux.right;
						}
					}
				}
			}

			float currentWaterHeight = model.waterHeights[x][y];
			float nextWaterHeight = model.waterHeights[x][y] + (dt / (length * length)) * ((finX + finY) - (foutX + foutY));
			model.waterHeights[x][y] = nextWaterHeight;

			if (x == 0 || y == 0 || x == model.size - 1 || y == model.size - 1) 
			{
				model.velocities[x][y] = glm::vec2(0, 0);				
			}
			else
			{

				float wX = (model.outflowFlux[x - 1][y].right - model.outflowFlux[x][y].left + model.outflowFlux[x][y].right - model.outflowFlux[x + 1][y].left) / 2;
				float wY = (model.outflowFlux[x][y - 1].top - model.outflowFlux[x][y].bottom + model.outflowFlux[x][y].top - model.outflowFlux[x][y + 1].bottom) / 2;


				float avgWaterHeight = (currentWaterHeight + nextWaterHeight) / 2;
				float xVelocity = (wX / length * avgWaterHeight);
				float yVelocity = (wY / length * avgWaterHeight);
				model.velocities[x][y] = glm::vec2(xVelocity, yVelocity);
			}

		}
	}
}
void sedimentDeposition(float dt)
{
	for (int y = 0; y < mapSize + 1; y++)
	{
		for (int x = 0; x < mapSize + 1; x++)
		{
			float tiltAngle = acosf(glm::dot(terrainMesh->getNormalAtPosition(x,y), glm::vec3(0,1,0)));
			float mag = glm::length(model.velocities[x][y]);
			
			float sedimentTransportCapacity = mag * sedimentCapacity * sinf(std::min(tiltAngle, 0.05f));

			if (model.suspendedSedimentAmounts[x][y] < sedimentTransportCapacity)
			{
				//take sediment
				float diff = dt * 0.5f * (sedimentTransportCapacity - model.suspendedSedimentAmounts[x][y]);
				model.terrainHeights[x][y] -= diff;
				model.suspendedSedimentAmounts[x][y] += diff;
				//model.waterHeights[x][y] += diff;

			}
			else
			{
				float diff = dt * (model.suspendedSedimentAmounts[x][y] - sedimentTransportCapacity);
				model.terrainHeights[x][y] += diff;
				model.suspendedSedimentAmounts[x][y] -= diff;
				//model.waterHeights[x][y] -= diff;
			}
		}
	}
}
void transportSediments(float dt)
{
	for (int y = 0; y < model.size; y++)
	{
		for (int x = 0; x < model.size; x++)
		{
			int x1 = x - model.velocities[x][y].x * dt;
			int y1 = y - model.velocities[x][y].y * dt;

			if (model.getCell(x1, y1) != nullptr)
				model.suspendedSedimentAmounts[x][y] = model.suspendedSedimentAmounts[x1][y1];
		}
	}
}
void evaporate(float dt)
{
	for (int y = 0; y < mapSize + 1; y++)
	{
		for (int x = 0; x < mapSize + 1; x++)
		{
			model.waterHeights[x][y] *= 1 - (simulationSpeed * evaporationRate * dt);
		}
	}
}

void updateModel(float dt)
{
	// each frame, water should uniformly increment accross the grid
	addWater(dt);

	// then calculate the outflow of water to other cells
	calculateModelOutflowFlux(dt);

	// receive water from neighbors and send out to neighbors
	calculateModelWaterHeights(dt);

	sedimentDeposition(dt);


	transportSediments(dt);

	evaporate(dt);

	terrainMesh->updateMeshFromHeights(&model.terrainHeights);
	waterMesh->updateMeshFromHeights(&model.terrainHeights, &model.waterHeights, &model.velocities);
}


int main()
{
	initModel();

	terrainMesh = new TerrainMesh(map.getheightMapSize(), &model.terrainHeights, mainShader);
	waterMesh = new WaterMesh(map.getheightMapSize(), &model.terrainHeights, &model.waterHeights, waterShader);

	map.saveHeightMapPPM("heightMap.ppm");

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

		if (window.getKeyDown(GLFW_KEY_ENTER)) {
			isModelRunning = !isModelRunning;
			printf("Model is %s\n", isModelRunning ? "Enabled" : "Disabled");
		}

		if (window.getKeyDown(GLFW_KEY_P)) {
			isRaining = !isRaining;
			printf("%s Rain\n", isRaining ? "Enabled" : "Disabled");
		}

		if (isModelRunning)
		{
			updateModel(0.016777f);
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
		waterShader.setUniformVector3("viewerPosition", camera.getPosition());
		waterShader.setUniformFloat("deltaTime", &deltaTime);

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