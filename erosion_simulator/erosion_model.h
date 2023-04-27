#pragma once
#include <glm/glm.hpp>
#include <vector>

enum class TerrainDebugMode
{
	TERRAIN_NORMAL,
	TERRAIN_SUSCEPTIBILITY,
	TERRAIN_SEDIMENT_DEPOSITION,
	TERRAIN_INVISIBLE,
	COUNT,
};

enum class WaterDebugMode
{
	WATER_NORMAL,
	WATER_VELOCITY,
	WATER_SEDIMENT_TRANSPORT,
	WATER_INVISIBLE,
	COUNT,
};

enum class PaintMode
{
	WATER_ADD,
	WATER_REMOVE,
	TERRAIN_ADD,
	TERRAIN_REMOVE,
	WATER_SOURCE,
	COUNT,
};

enum class WaveDirection
{
	NORTH,
	SOUTH,
	EAST,
	WEST,
	COUNT,
};

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

struct WaterSource
{
	glm::vec3 position;
	float radius;
	float intensity;
};

struct ErosionModel
{
	int width;
	int length;

	int simulationSpeed = 1;

	int rainIntensity = 1;
	int rainAmount = 1;
	float evaporationRate = 0.02f;

	float fluidDensity = 1.0f;
	// dont know if there should be more
	float lx = 1.0f;
	float ly = 1.0f;
	float area = lx * ly;

	float sedimentCapacity = 0.1f;
	float maxErosionDepth = 10.f;
	float slippageAngle;
	float seaLevel = -20;

	bool useSedimentSlippage = true;

	bool isRaining = false;
	bool isModelRunning = false;

	bool castRays = false;
	float brushRadius = 5.0f;
	float brushIntensity = 25.0f;
	PaintMode paintMode = PaintMode::WATER_ADD;
	WaterDebugMode waterDebugMode = WaterDebugMode::WATER_NORMAL;
	TerrainDebugMode terrainDebugMode = TerrainDebugMode::TERRAIN_NORMAL;

	bool generateWaves = false;
	float waveStrength = 5.f;
	float waveInterval = 1.f;
	WaveDirection waveDirection = WaveDirection::NORTH;

	std::vector<WaterSource> waterSources = std::vector<WaterSource>(0);

	float** terrainHeights; // b
	float** waterHeights; // d
	float** suspendedSedimentAmounts; // s
	FlowFlux** outflowFlux; // f
	glm::vec2** velocities; // v
	float** terrainHardness;

	ErosionModel(int width, int length)
		: width(width), length(length) {
		simulationSpeed = 1;

		rainIntensity = 1;
		rainAmount = 1;
		evaporationRate = 0.02f;

		fluidDensity = 1.0f;
		// dont know if there should be more
		lx = 1.0f;
		ly = 1.0f;
		area = lx * ly;

		sedimentCapacity = 0.01f;
		maxErosionDepth = 10.0f;
		slippageAngle = 45.0f;

		waterSources = std::vector<WaterSource>(0);


		terrainHeights = new float* [width];
		waterHeights = new float* [width];
		suspendedSedimentAmounts = new float* [width];
		outflowFlux = new FlowFlux * [width];
		velocities = new glm::vec2 * [width];
		terrainHardness = new float* [width];

		for (int i = 0; i < width; i++)
		{
			terrainHeights[i] = new float[length];
			waterHeights[i] = new float[length];
			suspendedSedimentAmounts[i] = new float[length];
			outflowFlux[i] = new FlowFlux[length];
			velocities[i] = new glm::vec2[length];
			terrainHardness[i] = new float[length];
		}
	}

	ErosionCell* getCell(int x, int y) {
		if (x < 0 || x >= width || y < 0 || y >= length)
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

	void ToggleModelRunning()
	{
		isModelRunning = !isModelRunning;
		printf("Model is %s\n", isModelRunning ? "Enabled" : "Disabled");	
	}

	void ToggleModelRaining()
	{
		isRaining = !isRaining;
		printf("%s Rain\n", isRaining ? "Enabled" : "Disabled");
	}

	void TogglePaintMode()
	{
		int current = (int)paintMode;
		if (current >= (int)PaintMode::COUNT - 1)
			paintMode = static_cast<PaintMode>(0);
		else
			paintMode = static_cast<PaintMode>(current + 1);
	}

	void ToggleTerrainDebugMode()
	{
		int current = (int)terrainDebugMode;
		if (current >= (int)TerrainDebugMode::COUNT - 1)
			terrainDebugMode = static_cast<TerrainDebugMode>(0);
		else
			terrainDebugMode = static_cast<TerrainDebugMode>(current + 1);
		printf("Terrain Debugging mode %d Enabled\n", (int)terrainDebugMode);
	}

	void ToggleWaterDebugMode()
	{
		int current = (int)waterDebugMode;
		if (current >= (int)WaterDebugMode::COUNT - 1)
			waterDebugMode = static_cast<WaterDebugMode>(0);
		else
			waterDebugMode = static_cast<WaterDebugMode>(current + 1);
		printf("Water Debugging mode %d Enabled\n", (int)waterDebugMode);
	}
};