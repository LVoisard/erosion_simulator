#include "height_map.h"
#include "external/simpleppm.h"

#include <vector>
#include <iostream>


HeightMap::HeightMap(int size, double minHeight, double maxHeight, double random)
	:size(size + 1), minHeight(minHeight), maxHeight(maxHeight), random(random)
{
	seedDistr = std::uniform_int_distribution<int>(INT_MIN, INT_MAX);
	heightDistr = std::uniform_real_distribution<>(minHeight, maxHeight);
	randomDistr = std::uniform_real_distribution<>(-random, random);
	generateHeightMap();
}

void HeightMap::setHeightRange(double minHeight, double maxHeight)
{
	this->minHeight = minHeight;
	this->maxHeight = maxHeight;
	heightDistr = std::uniform_real_distribution<>(minHeight, maxHeight);
	regenerateHeightMap();
}

void HeightMap::setRandomRange(double random)
{
	this->random = random;
	randomDistr = std::uniform_real_distribution<>(-random, random);
	regenerateHeightMap();
}

void HeightMap::printMap()
{
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			if (heightMap[x][y] > 0)
				std::cout << heightMap[x][y] << " ";
			else
				std::cout << "." << " ";
		}

		std::cout << std::endl;
	}
}

void HeightMap::saveHeightMapPPM(std::string fileName)
{
	std::vector<double> buffer(3 * size * size);

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			double color = getRGBA(x, y);
			buffer[3 * y * size + 3 * x + 0] = color;
			buffer[3 * y * size + 3 * x + 1] = color;
			buffer[3 * y * size + 3 * x + 2] = color;
		}
	}

	save_ppm(fileName, buffer, size, size);
	buffer.clear();
}

void HeightMap::generateHeightMap()
{
	heightMap = new double* [size];
	for (int i = 0; i < size; i++) {
		heightMap[i] = new double[size];
	}

	double num1 = heightMap[0][0] = (double)heightDistr(mapGenerator);
	double num2 = heightMap[size - 1][0] = (double)heightDistr(mapGenerator);
	double num3 = heightMap[0][size - 1] = (double)heightDistr(mapGenerator);
	double num4 = heightMap[size - 1][size - 1] = (double)heightDistr(mapGenerator);

	int chunkSize = size - 1;
	double roughness = random;

	while (chunkSize > 1) {
		int half = chunkSize / 2;
		squareStep(chunkSize, half);
		diamondStep(chunkSize, half);
		chunkSize /= 2;
		roughness /= 2.0;
		randomDistr = std::uniform_real_distribution<>(-roughness, roughness);
	}

}

void HeightMap::regenerateHeightMap()
{
	for (int i = 0; i < size; i++) {
		delete heightMap[i];
	}
	delete heightMap;
	generateHeightMap();
}

void HeightMap::squareStep(int chunkSize, int halfChunkSize)
{
	for (int x = 0; x < size - 1; x += chunkSize)
	{
		for (int y = 0; y < size - 1; y += chunkSize)
		{
			double avg = heightMap[x][y] + heightMap[x + chunkSize][y] + heightMap[x][y + chunkSize] + heightMap[x + chunkSize][y + chunkSize];
			avg /= 4.0;
			heightMap[x + halfChunkSize][y + halfChunkSize] = avg + randomDistr(mapGenerator);
		}
	}
}

void HeightMap::diamondStep(int chunkSize, int halfChunkSize)
{
	for (int x = 0; x < size - 1; x += halfChunkSize)
	{
		for (int y = (x + halfChunkSize) % chunkSize; y < size - 1; y += chunkSize)
		{
			double avg = heightMap[(x - halfChunkSize + size - 1) % (size - 1)][y] +
				heightMap[(x + halfChunkSize) % (size - 1)][y] +
				heightMap[x][(y + halfChunkSize) % (size - 1)] +
				heightMap[x][(y - halfChunkSize + size - 1) % (size - 1)];
			avg /= 4.0 + randomDistr(mapGenerator);
			heightMap[x][y] = avg;

			if (x == 0) heightMap[size - 1][y] = avg;
			if (y == 0) heightMap[x][size - 1] = avg;
		}
	}
}
