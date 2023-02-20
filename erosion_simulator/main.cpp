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
#include "imgui.h"

#define GLM_FORCE_RADIANS
const float GRAVITY_ACCELERATION = 9.807f;


const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

enum PaintMode
{
	WATER_ADD,
	WATER_REMOVE,
	TERRAIN_ADD,
	TERRAIN_REMOVE,
	COUNT,
};

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
int mapSize = 256;
float minHeight = 0;
float maxHeight = 100;
float random = 2;
HeightMap map(minHeight, maxHeight);

TerrainMesh* terrainMesh;
WaterMesh* waterMesh;

ErosionModel* erosionModel;

std::default_random_engine gen;
std::uniform_int_distribution<> distr;

float fov = 90.0f;
bool castRays = false;
glm::vec3 cursorOverPosition = glm::vec3(INT_MIN);
float brushRadius = 5.0f;
float brushIntensity = 25.0f;
PaintMode paintMode = WATER_ADD;

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
			erosionModel->waterHeights[x][y] = 0.0f;
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
			erosionModel->waterHeights[x][y] = 0.0f;
			erosionModel->suspendedSedimentAmounts[x][y] = 0.0f;
			erosionModel->outflowFlux[x][y] = FlowFlux{};
			erosionModel->velocities[x][y] = glm::vec2(0.0f);
			erosionModel->terrainHardness[x][y] = 0.1f;
		}
	}

	terrainMesh->updateOriginalHeights(&erosionModel->terrainHeights);
	terrainMesh->updateMeshFromHeights(&erosionModel->terrainHeights);
	waterMesh->updateMeshFromHeights(&erosionModel->terrainHeights, &erosionModel->waterHeights, &erosionModel->velocities);
}
void addPrecipitation(float dt) {
	if (erosionModel->isRaining) {
		for (int y = 0; y < erosionModel->length; y++)
		{
			for (int x = 0; x < erosionModel->width; x++)
			{
				if (distr(gen) <= erosionModel->rainAmount * mapSize)
					erosionModel->waterHeights[x][y] += dt * erosionModel->rainIntensity * erosionModel->simulationSpeed;
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
				if (glm::length(glm::vec2(cursorOverPosition.x, cursorOverPosition.z) - glm::vec2(mapPos.x, mapPos.z)) < brushRadius)
				{
					switch (paintMode)
					{
					case WATER_ADD:
						erosionModel->waterHeights[x][y] += dt * brushIntensity; 
						break;
					case WATER_REMOVE:
						erosionModel->waterHeights[x][y] -= dt * brushIntensity;
						erosionModel->waterHeights[x][y] = std::max(erosionModel->waterHeights[x][y], 0.0f);
						break;
					case TERRAIN_ADD:
						erosionModel->terrainHeights[x][y] += dt * brushIntensity;// *(1 - glm::length(cursorOverPosition - mapPos) / brushRadius);
						break;
					case TERRAIN_REMOVE:
						erosionModel->terrainHeights[x][y] -= dt * brushIntensity;//  * (1 - glm::length(cursorOverPosition - mapPos) / brushRadius);
						break;
					default:
						throw std::exception("invalid paint mode");
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

			float sedimentTransportCapacity = mag * erosionModel->sedimentCapacity * std::min(sinf(tiltAngle), 0.05f);

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

			x1 = prevX < 0 ? std::floor(prevX) : std::ceil(prevX);
			y1 = prevY < 0 ? std::floor(prevY) : std::ceil(prevY);

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

	for (int y = 0; y < erosionModel->length; y++)
	{
		delete[] temp[y];
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
	waterMesh->updateMeshFromHeights(&erosionModel->terrainHeights, &erosionModel->waterHeights, &erosionModel->velocities);
}


int main()
{
	map.createProceduralHeightMap(mapSize, random);
	// map.loadHeightMapFromFile("C:\\Users\\laure\\Downloads\\heightmapper-1676501778575.png");

	distr = std::uniform_int_distribution(0, map.getWidth() * map.getLength());
	erosionModel = new ErosionModel(map.getWidth(), map.getLength());
	initModel();

	terrainMesh = new TerrainMesh(map.getWidth(), map.getLength(), &erosionModel->terrainHeights, mainShader);
	waterMesh = new WaterMesh(map.getWidth(), map.getLength(), &erosionModel->terrainHeights, &erosionModel->waterHeights, waterShader);

	map.saveHeightMapPPM("heightMap.ppm");

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

		// disable until update all meshes is done
		if (window.getKeyDown(GLFW_KEY_SPACE)) {
			castRays = !castRays;
			if(!castRays)
				cursorOverPosition = glm::vec3(INT_MIN);
		}

		if (window.getKeyDown(GLFW_KEY_R)) {
			map.changeSeed();
			resetModel();
		}

		if (window.getKeyDown(GLFW_KEY_ENTER)) {
			erosionModel->isModelRunning = !erosionModel->isModelRunning;
			printf("Model is %s\n", erosionModel->isModelRunning ? "Enabled" : "Disabled");
		}

		if (window.getKeyDown(GLFW_KEY_P)) {
			erosionModel->isRaining = !erosionModel->isRaining;
			printf("%s Rain\n", erosionModel->isRaining ? "Enabled" : "Disabled");
		}

		if (window.getKeyDown(GLFW_KEY_V)) {
			erosionModel->debugWaterVelocity = !erosionModel->debugWaterVelocity;
			printf("%s Velocity Debugging\n", erosionModel->debugWaterVelocity ? "Enabled" : "Disabled");
		}

		if (window.getKeyDown(GLFW_KEY_TAB)) {
			int current = (int)paintMode;
			if (current >= COUNT - 1)
				paintMode = static_cast<PaintMode>(0);
			else
				paintMode = static_cast<PaintMode>(current + 1);
		}

		if (erosionModel->isModelRunning)
		{
			//printf("Frame time: %f\n", deltaTime);
			//printf("Time to render 1 simulation second: %f\n", deltaTime * 60.f);
			updateModel(deltaTime);
		}

		if (window.getMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
		{
			camera.toggleViewMode();
			if (camera.inFreeView())
				cursorOverPosition = glm::vec3(INT_MIN);
		}

		if (window.getMouseScrollY() != 0 && !camera.inFreeView())
		{
			brushRadius += window.getMouseScrollY();
			brushRadius = std::clamp(brushRadius, 1.0f, 50.0f);
		}

		camera.update(deltaTime);

		if (castRays && !camera.inFreeView())
			raycastThroughScene();

		

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = camera.getViewMatrix();

		skybox.DrawSkybox(view, proj);

		// draw our first triangle
		mainShader.use();
		mainShader.setMat4("model", model);
		mainShader.setMat4("view", view);
		mainShader.setMat4("projection", proj);
		mainShader.setUniformBool("checkMousePos", castRays);
		mainShader.setUniformVector3("cursorOverTerrainPos", cursorOverPosition);
		mainShader.setUniformFloat("brushRadius", &brushRadius);


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
		waterShader.setUniformBool("debugWaterVelocity", erosionModel->debugWaterVelocity);
		waterNormalTexture.use();
		waterShader.setTexture("texture0", GL_TEXTURE0);


		waterMesh->draw();
		waterShader.stop();

		window.Menu(erosionModel);
		// window.IMGuiTest();


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