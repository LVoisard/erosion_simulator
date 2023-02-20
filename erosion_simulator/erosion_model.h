#pragma once
#include <glm/glm.hpp>


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
	float slippageAngle;

	bool useSedimentSlippage = true;

	bool isRaining = false;
	bool isModelRunning = false;

	bool debugWaterVelocity = false;

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

		sedimentCapacity = 0.1f;
		slippageAngle = 45.0f;


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
};