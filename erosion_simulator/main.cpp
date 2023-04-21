#include "simulation_parameters_ui.h"
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
#include "external/simpleppm.h"

#include <iostream>

#include <random>
#include "imgui.h"

#define GLM_FORCE_RADIANS
const float GRAVITY_ACCELERATION = 9.807f;

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

float waterflowRate = 0.1f;

Window window(SCR_WIDTH, SCR_HEIGHT);
Shader mainShader("shaders/main.vert", "shaders/main.frag");
Shader waterShader("shaders/water.vert", "shaders/water.frag");
Camera camera(&window, 5.0f, .25f, .5f);

Texture grassTexture("textures/grass.jpg");
Texture sandTexture("textures/sand.jpg");
Texture rockTexture("textures/rock.jpg");
Texture waterNormalTexture("textures/water-normal-map.jpg");

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
int mapSize = 512;
float minHeight = -128;
float maxHeight = 128;
float random = 4;
HeightMap map(minHeight, maxHeight);

TerrainMesh* terrainMesh;
WaterMesh* waterMesh;

ErosionModel* erosionModel;
SimulationParametersUI* simParams;

std::default_random_engine gen;
std::uniform_int_distribution<> distr;

float fov = 90.0f;
glm::vec3 cursorOverPosition = glm::vec3(INT_MIN);

void raycastThroughScene()
{
	float pixelSize = (2 * tanf(fov) / 2 / window.getHeight());
	glm::vec3 A = camera.getPosition() - camera.getLookAt();
	glm::vec3 up = camera.getUp();
	glm::vec3 right = camera.getRight();
	glm::vec3 B = (A + tanf(fov) / 2 * up);
	glm::vec3 C = B - (window.getWidth() / 2 * pixelSize * right);

	glm::vec3 direction = glm::normalize(
		C +
		((window.getMousePosX() * pixelSize + pixelSize / 2.0f) * camera.getRight()) -
		((window.getMousePosY() * pixelSize + pixelSize / 2.0f) * camera.getUp() + camera.getPosition()));

	cursorOverPosition = glm::vec3(INT_MIN);

	for (int i = 0; i < terrainMesh->indexCount; i += 3)
	{
		glm::vec3 triA = terrainMesh->vertices[terrainMesh->indices[i]].pos;
		glm::vec3 triB = terrainMesh->vertices[terrainMesh->indices[i + 1]].pos;
		glm::vec3 triC = terrainMesh->vertices[terrainMesh->indices[i + 2]].pos;
		glm::vec3 normal = glm::normalize(glm::cross(triC - triB, triA - triB));
		float t = glm::dot(triA - camera.getPosition(), normal) / glm::dot(direction, normal);

		glm::vec3 P = camera.getPosition() + t * direction;

		//check if in triangle
		float Sbc = glm::dot(glm::cross(triC - triB, P - triB), normal);
		float Sca = glm::dot(glm::cross(triA - triC, P - triC), normal);
		float Sab = glm::dot(glm::cross(triB - triA, P - triA), normal);

		if (Sbc >= 0 && Sca >= 0 && Sab >= 0)
		{
			cursorOverPosition = P;
			return;
		}
	}
}

void initModel()
{
	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			erosionModel->terrainHeights[x][y] = map.samplePoint(x, y);
			erosionModel->waterHeights[x][y] = erosionModel->seaLevel > erosionModel->terrainHeights[x][y] ? erosionModel->seaLevel - erosionModel->terrainHeights[x][y] : 0.0f;
			erosionModel->suspendedSedimentAmounts[x][y] = 0.0f;
			erosionModel->outflowFlux[x][y] = FlowFlux{};
			erosionModel->velocities[x][y] = glm::vec2(0.0f);
			erosionModel->terrainHardness[x][y] = 0.1f;
		}
	}
}
void resetModel()
{
	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			erosionModel->terrainHeights[x][y] = map.samplePoint(x, y);
			erosionModel->waterHeights[x][y] = erosionModel->seaLevel > erosionModel->terrainHeights[x][y] ? erosionModel->seaLevel - erosionModel->terrainHeights[x][y] : 0.0f;;
			erosionModel->suspendedSedimentAmounts[x][y] = 0.0f;
			erosionModel->outflowFlux[x][y] = FlowFlux{};
			erosionModel->velocities[x][y] = glm::vec2(0.0f);
			erosionModel->terrainHardness[x][y] = 0.1f;
		}
	}

	terrainMesh->updateOriginalHeights(&erosionModel->terrainHeights);
	terrainMesh->updateMeshFromHeights(&erosionModel->terrainHeights);
	waterMesh->updateMeshFromHeights(&erosionModel->terrainHeights, &erosionModel->waterHeights, &erosionModel->velocities, &erosionModel->suspendedSedimentAmounts);
}
void addPrecipitation(float dt) {

	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			// adjust sea level minimum water amount
			if (erosionModel->waterHeights[x][y] + erosionModel->terrainHeights[x][y] < erosionModel->seaLevel)
				erosionModel->waterHeights[x][y] += dt;
			if (erosionModel->isRaining) {
				if (distr(gen) <= erosionModel->rainAmount * erosionModel->width)
					erosionModel->waterHeights[x][y] += dt * erosionModel->rainIntensity * erosionModel->simulationSpeed;
			}

			for (WaterSource waterSource : erosionModel->waterSources)
			{
				glm::vec3 mapPos = terrainMesh->getPositionAtIndex(x, y);
				if (glm::length(glm::vec2(waterSource.position.x, waterSource.position.z) - glm::vec2(mapPos.x, mapPos.z)) < waterSource.radius)
				{
					erosionModel->waterHeights[x][y] += dt * waterSource.intensity;
				}
			}
		}
	}
}
void paint(float dt) {
	if (window.getMouseButton(GLFW_MOUSE_BUTTON_LEFT))
	{
		for (int y = 0; y < erosionModel->length; y++)
		{
			for (int x = 0; x < erosionModel->width; x++)
			{
				glm::vec3 mapPos = terrainMesh->getPositionAtIndex(x, y);
				if (glm::length(glm::vec2(cursorOverPosition.x, cursorOverPosition.z) - glm::vec2(mapPos.x, mapPos.z)) < erosionModel->brushRadius)
				{
					switch (erosionModel->paintMode)
					{
					case PaintMode::WATER_ADD:
						erosionModel->waterHeights[x][y] += dt * erosionModel->brushIntensity;
						break;
					case PaintMode::WATER_REMOVE:
						erosionModel->waterHeights[x][y] -= dt * erosionModel->brushIntensity;
						erosionModel->waterHeights[x][y] = std::max(erosionModel->waterHeights[x][y], 0.0f);
						break;
					case PaintMode::TERRAIN_ADD:
						erosionModel->terrainHeights[x][y] += dt * erosionModel->brushIntensity;// *(1 - glm::length(cursorOverPosition - mapPos) / brushRadius);
						break;
					case PaintMode::TERRAIN_REMOVE:
						erosionModel->terrainHeights[x][y] -= dt * erosionModel->brushIntensity;//  * (1 - glm::length(cursorOverPosition - mapPos) / brushRadius);
						break;
					default:
						break;
					}
				}
			}
		}
	}

	if (window.getMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
	{
		if (erosionModel->paintMode == PaintMode::WATER_SOURCE)
		{
			for (int y = 0; y < erosionModel->length; y++)
			{
				for (int x = 0; x < erosionModel->width; x++)
				{
					glm::vec3 mapPos = terrainMesh->getPositionAtIndex(x, y);
					if (glm::length(glm::vec2(cursorOverPosition.x, cursorOverPosition.z) - glm::vec2(mapPos.x, mapPos.z)) < 0.5f)
					{
						WaterSource source = WaterSource();
						source.position = mapPos;
						source.intensity = erosionModel->brushIntensity;
						source.radius = erosionModel->brushRadius;

						erosionModel->waterSources.push_back(source);
					}
				}
			}			
		}
	}
}
void calculateModelOutflowFlux(float dt)
{
	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					if (abs(i) == abs(j)) continue;
					float f = 0.0f;
					if (erosionModel->getCell(x + i, y + j) != nullptr) {
						float dHeight = erosionModel->terrainHeights[x][y] + erosionModel->waterHeights[x][y] - (erosionModel->terrainHeights[x + i][y + j] + erosionModel->waterHeights[x + i][y + j]);
						float dPressure = erosionModel->fluidDensity * GRAVITY_ACCELERATION * dHeight;
						float acceleration = dPressure / (erosionModel->fluidDensity * erosionModel->lx);
						f = dt * erosionModel->simulationSpeed * erosionModel->area * acceleration;
					}

					// compute flux
					if (j == -1) {
						erosionModel->outflowFlux[x][y].bottom = std::max(0.0f, erosionModel->outflowFlux[x][y].bottom + f);
					}
					else if (j == 1) {
						erosionModel->outflowFlux[x][y].top = std::max(0.0f, erosionModel->outflowFlux[x][y].top + f);
					}
					else if (i == -1) {
						erosionModel->outflowFlux[x][y].left = std::max(0.0f, erosionModel->outflowFlux[x][y].left + f);
					}
					else if (i == 1) {
						erosionModel->outflowFlux[x][y].right = std::max(0.0f, erosionModel->outflowFlux[x][y].right + f);
					}

					// rescale
					if (j == -1) {
						erosionModel->outflowFlux[x][y].bottom *= std::min(1.0f, erosionModel->waterHeights[x][y] * erosionModel->area / (erosionModel->outflowFlux[x][y].bottom * dt));
					}
					else if (j == 1) {
						erosionModel->outflowFlux[x][y].top *= std::min(1.0f, erosionModel->waterHeights[x][y] * erosionModel->area / (erosionModel->outflowFlux[x][y].top * dt));
					}
					else if (i == -1) {
						erosionModel->outflowFlux[x][y].left *= std::min(1.0f, erosionModel->waterHeights[x][y] * erosionModel->area / (erosionModel->outflowFlux[x][y].left * dt));
					}
					else if (i == 1) {
						erosionModel->outflowFlux[x][y].right *= std::min(1.0f, erosionModel->waterHeights[x][y] * erosionModel->area / (erosionModel->outflowFlux[x][y].right * dt));
					}
				}
			}

			erosionModel->outflowFlux[x][y].bottom = std::max(0.0f, erosionModel->outflowFlux[x][y].bottom);
			erosionModel->outflowFlux[x][y].top = std::max(0.0f, erosionModel->outflowFlux[x][y].top);
			erosionModel->outflowFlux[x][y].left = std::max(0.0f, erosionModel->outflowFlux[x][y].left);
			erosionModel->outflowFlux[x][y].right = std::max(0.0f, erosionModel->outflowFlux[x][y].right);
		}
	}
}
void calculateModelWaterHeights(float dt)
{
	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			float finX = 0.0f;
			float finY = 0.0f;
			float foutX = erosionModel->outflowFlux[x][y].left + erosionModel->outflowFlux[x][y].right;
			float foutY = erosionModel->outflowFlux[x][y].top + erosionModel->outflowFlux[x][y].bottom;

			float finL = 0.0f;
			float finR = 0.0f;
			float finT = 0.0f;
			float finB = 0.0f;

			float foutL = erosionModel->outflowFlux[x][y].left;
			float foutR = erosionModel->outflowFlux[x][y].right;
			float foutT = erosionModel->outflowFlux[x][y].top;
			float foutB = erosionModel->outflowFlux[x][y].bottom;

			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					if (abs(i) == abs(j)) continue;
					if (erosionModel->getCell(x - i, y - j) != nullptr) {
						FlowFlux flux = erosionModel->outflowFlux[x - i][y - j];
						if (j == -1) {
							finY += flux.bottom;
							finB = flux.bottom;
						}
						else if (j == 1) {
							finY += flux.top;
							finT = flux.top;
						}
						else if (i == -1) {
							finX += flux.left;
							finL = flux.left;
						}
						else if (i == 1) {
							finX += flux.right;
							finR = flux.right;
						}
					}
				}
			}

			float currentWaterHeight = erosionModel->waterHeights[x][y];
			float nextWaterHeight = currentWaterHeight + dt * ((finX + finY) - (foutX + foutY)) / (erosionModel->area);

			float wX = (finR - foutL + foutR - finL) / 2;
			float wY = (finT - foutB + foutT - finB) / 2;


			float avgWaterHeight = (currentWaterHeight + nextWaterHeight) / 2;

			float xVelocity = (wX / (erosionModel->lx));
			float yVelocity = (wY / (erosionModel->ly));

			if (abs(avgWaterHeight) < 1) {
				xVelocity *= avgWaterHeight;
				yVelocity *= avgWaterHeight;
			}
			else
			{
				xVelocity /= avgWaterHeight;
				yVelocity /= avgWaterHeight;
			}

			erosionModel->velocities[x][y] = glm::vec2(xVelocity, yVelocity);
			erosionModel->waterHeights[x][y] = nextWaterHeight;
		}
	}
}
void sedimentDeposition(float dt)
{
	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			float tiltAngle = acosf(glm::dot(terrainMesh->getNormalAtIndex(x, y), glm::vec3(0, 1, 0)));
			float mag = glm::length(erosionModel->velocities[x][y]);

			float lmax = std::clamp(1 - std::max(0.f, erosionModel->maxErosionDepth - erosionModel->waterHeights[x][y]) / erosionModel->maxErosionDepth, 0.f, 1.f);
			float sedimentTransportCapacity = mag * erosionModel->sedimentCapacity * std::max(sinf(tiltAngle), 0.05f) * lmax;

			if (erosionModel->suspendedSedimentAmounts[x][y] < sedimentTransportCapacity)
			{
				//take sediment
				float diff = dt * 0.5f * (sedimentTransportCapacity - erosionModel->suspendedSedimentAmounts[x][y]);
				erosionModel->terrainHeights[x][y] -= diff;
				erosionModel->suspendedSedimentAmounts[x][y] += diff;
			}
			else if (erosionModel->suspendedSedimentAmounts[x][y] > sedimentTransportCapacity)
			{
				float diff = dt * (erosionModel->suspendedSedimentAmounts[x][y] - sedimentTransportCapacity);
				erosionModel->terrainHeights[x][y] += diff;
				erosionModel->suspendedSedimentAmounts[x][y] -= diff;
			}
		}
	}
}
void transportSediments(float dt)
{
	float** temp = new float* [erosionModel->width];
	for (int i = 0; i < erosionModel->width; i++)
	{
		temp[i] = new float[erosionModel->length];
	}

	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			temp[x][y] = erosionModel->suspendedSedimentAmounts[x][y];

			float prevX = x - erosionModel->velocities[x][y].x * dt;
			float prevY = y - erosionModel->velocities[x][y].y * dt;

			int x1 = x;
			int y1 = y;

			if ((erosionModel->velocities[x][y].y) / (erosionModel->velocities[x][y].x) > 0.293f)
				x1 = prevX < x ? std::floor(prevX) : std::ceil(prevX);

			if ((erosionModel->velocities[x][y].x) / (erosionModel->velocities[x][y].y) > 0.293f)
				y1 = prevY < y ? std::floor(prevY) : std::ceil(prevY);

			if (erosionModel->getCell(x1, y1) != nullptr)
				temp[x][y] = erosionModel->suspendedSedimentAmounts[x1][y1];
		}
	}

	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			erosionModel->suspendedSedimentAmounts[x][y] = temp[x][y];
		}
	}

	for (int x = 0; x < erosionModel->width; x++)
	{
		delete[] temp[x];
	}
	delete[] temp;
}
void sedimentSlippage(float dt)
{
	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					if (abs(i) == abs(j)) continue;
					if (erosionModel->getCell(x - i, y - j) != nullptr) {
						float dh =
							(erosionModel->terrainHeights[x][y]) -
							(erosionModel->terrainHeights[x - i][y - j]);
						float talus = erosionModel->lx * tanf(glm::radians(erosionModel->slippageAngle));
						if (dh > talus)
						{
							float slippage = dt * (dh - talus);
							erosionModel->terrainHeights[x][y] -= slippage;
							erosionModel->terrainHeights[x - i][y - j] += slippage;
						}
					}
				}
			}
		}
	}
}
void evaporate(float dt)
{
	for (int y = 0; y < erosionModel->length; y++)
	{
		for (int x = 0; x < erosionModel->width; x++)
		{
			//only evaporate above sea level
			if (erosionModel->waterHeights[x][y] + erosionModel->terrainHeights[x][y] > erosionModel->seaLevel)
				erosionModel->waterHeights[x][y] *= 1 - (erosionModel->simulationSpeed * erosionModel->evaporationRate * dt);
		}
	}
}
void updateModel(float dt)
{
	paint(dt);

	// each frame, water should uniformly increment accross the grid
	addPrecipitation(dt);

	// then calculate the outflow of water to other cells
	calculateModelOutflowFlux(dt);

	// receive water from neighbors and send out to neighbors
	calculateModelWaterHeights(dt);

	sedimentDeposition(dt);

	transportSediments(dt);

	if (erosionModel->useSedimentSlippage)
		sedimentSlippage(dt);

	evaporate(dt);

	terrainMesh->updateMeshFromHeights(&erosionModel->terrainHeights);
	waterMesh->updateMeshFromHeights(&erosionModel->terrainHeights, &erosionModel->waterHeights, &erosionModel->velocities, &erosionModel->suspendedSedimentAmounts);
}

void HandleHeightmapResets()
{
	if (simParams->regenerateHeightMapRequested || window.getKeyDown(GLFW_KEY_R)) {
		simParams->regenerateHeightMapRequested = false;
		map.changeSeed();
		resetModel();
	}

	if (simParams->saveHeightMapRequested)
	{
		simParams->saveHeightMapRequested = false;
		std::vector<double> buffer(3 * map.getWidth() * map.getLength());

		for (int y = 0; y < map.getLength(); y++) {
			for (int x = 0; x < map.getWidth(); x++) {
				double color = std::clamp((double)(erosionModel->terrainHeights[x][y] + minHeight) / (double)(maxHeight + minHeight), 0.0, 1.0);
				buffer[3 * y * map.getWidth() + 3 * x + 0] = color;
				buffer[3 * y * map.getWidth() + 3 * x + 1] = color;
				buffer[3 * y * map.getWidth() + 3 * x + 2] = color;
			}
		}
		std::string fileName = std::string(simParams->fileSaveName) + ".ppm";
		save_ppm(fileName, buffer, map.getWidth(), map.getLength());
		buffer.clear();
	}


}
void HandleKeyboardInputs()
{
	if (window.getKeyDown(GLFW_KEY_SPACE)) {
		erosionModel->castRays = !erosionModel->castRays;
		if (!erosionModel->castRays)
			cursorOverPosition = glm::vec3(INT_MIN);
	}

	if (window.getKeyDown(GLFW_KEY_ENTER)) {
		erosionModel->ToggleModelRunning();
	}

	if (window.getKeyDown(GLFW_KEY_P)) {
		erosionModel->ToggleModelRaining();
	}

	if (window.getKeyDown(GLFW_KEY_V)) {
		erosionModel->ToggleWaterDebugMode();
	}

	if (window.getKeyDown(GLFW_KEY_B)) {
		erosionModel->ToggleTerrainDebugMode();
	}

	if (window.getKeyDown(GLFW_KEY_TAB) && erosionModel->castRays) {
		erosionModel->TogglePaintMode();
	}
}
void HandleCamera(float deltaTime)
{
	if (window.getMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
	{
		camera.toggleViewMode();
		if (camera.inFreeView())
			cursorOverPosition = glm::vec3(INT_MIN);
	}

	if (window.getMouseScrollY() != 0 && !camera.inFreeView())
	{
		erosionModel->brushRadius += window.getMouseScrollY();
		erosionModel->brushRadius = std::clamp(erosionModel->brushRadius, 1.0f, 50.0f);
	}

	camera.update(deltaTime);

	if (erosionModel->castRays && !camera.inFreeView())
		raycastThroughScene();
}

void UpdateShaders(glm::mat4& view, glm::mat4& proj, glm::mat4& model, float& deltaTime)
{
	skybox.DrawSkybox(view, proj);

	// draw our first triangle
	mainShader.use();
	mainShader.setMat4("model", model);
	mainShader.setMat4("view", view);
	mainShader.setMat4("projection", proj);
	mainShader.setUniformBool("checkMousePos", erosionModel->castRays);
	mainShader.setUniformVector3("cursorOverTerrainPos", cursorOverPosition);
	mainShader.setUniformFloat("brushRadius", &erosionModel->brushRadius);
	mainShader.setUniformInt("debugMode", (int)erosionModel->terrainDebugMode);


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
	waterShader.setUniformInt("waterDebugMode", (int)erosionModel->waterDebugMode);
	waterNormalTexture.use();
	waterShader.setTexture("texture0", GL_TEXTURE0);


	waterMesh->draw();
	waterShader.stop();
}

int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		printf("Invalid arguments, possible commands: \n");
		printf("default (n (1 - 11)) (randomness factor(0-4)) \n");
		printf("heightmap (filepath) \n");
		printf("obj (filepath) (slopeHeight)\n");
		return -1;
	}

	for (int i = 0; i < argc; i++)
	{
		printf(argv[i]);
	}
	if (std::string(argv[1]) == "default")
	{
		map.createProceduralHeightMap(std::pow(2, std::stoi(argv[2])), std::stoi(argv[3]));
	}
	else if (std::string(argv[1]) == "heightmap")
	{
		map.loadHeightMapFromFile(std::string(argv[2]));
	}
	else if (std::string(argv[1]) == "obj")
	{
		map.loadHeightMapFromOBJFile(std::string(argv[2]), argc == 4 ? std::stoi(argv[3]) : 0);
	}

	distr = std::uniform_int_distribution(0, map.getWidth() * map.getLength());
	erosionModel = new ErosionModel(map.getWidth(), map.getLength());
	simParams = new SimulationParametersUI(std::string(argv[1]) == "default");
	initModel();

	terrainMesh = new TerrainMesh(map.getWidth(), map.getLength(), &erosionModel->terrainHeights, mainShader);
	waterMesh = new WaterMesh(map.getWidth(), map.getLength(), &erosionModel->terrainHeights, &erosionModel->waterHeights, waterShader);

	terrainMesh->init();
	waterMesh->init();

	glm::mat4 proj = glm::mat4(1.0f);
	proj = glm::perspective(glm::radians(fov), window.getAspectRatio(), 0.1f, 1000.0f);

	auto currentTime = std::chrono::high_resolution_clock::now();
	while (!window.shouldWindowClose())
	{
		auto newTime = std::chrono::high_resolution_clock::now();
		float deltaTime =
			std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		currentTime = newTime;

		HandleHeightmapResets();
		// stop taking input
		if (!window.showSaveMenu) {
			HandleKeyboardInputs();
			HandleCamera(deltaTime);
		}

		if (erosionModel->isModelRunning)
		{
			//printf("Frame time: %f\n", deltaTime);
			//printf("Time to render 1 simulation second: %f\n", deltaTime * 60.f);
			updateModel(0.033333f);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = camera.getViewMatrix();

		UpdateShaders(view, proj, model, deltaTime);

		window.Menu(erosionModel, simParams);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		ImGui::EndFrame();

		window.swapBuffers();
		window.updateInput();
		window.pollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}
