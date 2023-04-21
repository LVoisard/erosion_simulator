#include "height_map.h"
#include "external/simpleppm.h"

#include <vector>
#include <iostream>

#include "external/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>


HeightMap::HeightMap(double minHeight, double maxHeight)
	:minHeight(minHeight), maxHeight(maxHeight)
{
}

void HeightMap::createProceduralHeightMap(int size, double random)
{
	this->width = size + 1;
	this->length = size + 1;
	this->random = random;
	seedDistr = std::uniform_int_distribution<int>(INT_MIN, INT_MAX);
	heightDistr = std::uniform_real_distribution<>(minHeight, maxHeight);
	randomDistr = std::uniform_real_distribution<>(-random, random);
	generateHeightMap();
}

void HeightMap::loadHeightMapFromFile(std::string fileName)
{
	int width, height, channels;
	unsigned char* img = stbi_load(fileName.c_str(), &width, &height, &channels, 1);
	if (img == NULL) {
		throw "Error reading the image";
	}
	printf("loaded heigthmap from file");

	this->width = width;
	this->length = height;

	heightMap = new double* [width];
	for (int i = 0; i < width; i++) {
		heightMap[i] = new double[height];
	}

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			const stbi_uc* p = img + (y * height + x);
			
			float h = p[0] / 255.0f * maxHeight;
			heightMap[x][y] = h;
		}
	}

	stbi_image_free(img);
}

void HeightMap::loadHeightMapFromOBJFile(std::string fileName, float heightDiff)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::vector<glm::vec3> vertices;
	std::vector<uint32_t> indices;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<glm::vec3, int> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			glm::vec3 vertex{};

			if (index.vertex_index >= 0) {
				vertex = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	int width = 0;
	int length = 0;

	float Xval = vertices[0].x;
	float Zval = vertices[0].z;

	for (auto vert : vertices)
	{
		if (vert.x == Xval) width++;
		if (vert.z == Zval) length++;
	}

	this->width = width;
	this->length = length;

	heightMap = new double* [width];
	for (int i = 0; i < width; i++) {
		heightMap[i] = new double[length];
	}

	for (int y = 0; y < length; y++)
	{
		for (int x = 0; x < width; x++)
		{
			heightMap[x][y] = vertices[y * width + x].y * 256.0 + (float)x / (width / heightDiff);
		}
	}
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
	for (int y = 0; y < length; y++) {
		for (int x = 0; x < width; x++) {
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
	std::vector<double> buffer(3 * width * length);

	for (int y = 0; y < length; y++) {
		for (int x = 0; x < width; x++) {
			double color = getRGBA(x, y);
			buffer[3 * y * width + 3 * x + 0] = color;
			buffer[3 * y * width + 3 * x + 1] = color;
			buffer[3 * y * width + 3 * x + 2] = color;
		}
	}

	save_ppm(fileName +".ppm", buffer, width, length);
	buffer.clear();
}

void HeightMap::saveHeightMapPPM(std::string fileName, float*** hmp)
{
	std::vector<double> buffer(3 * width * length);

	for (int y = 0; y < length; y++) {
		for (int x = 0; x < width; x++) {
			double color = std::clamp((double)(*hmp[x][y] + minHeight) / (double)(maxHeight + minHeight), 0.0, 1.0);
			buffer[3 * y * width + 3 * x + 0] = color;
			buffer[3 * y * width + 3 * x + 1] = color;
			buffer[3 * y * width + 3 * x + 2] = color;
		}
	}

	save_ppm(fileName + ".ppm", buffer, width, length);
	buffer.clear();
}

void HeightMap::generateHeightMap()
{
	if (width != length)
		throw std::exception("Width and Length of the heightmap do not match");

	int size = width;

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
	if (width != length)
		throw std::exception("Width and Lenght of the heightmap do not match");

	int size = width;
	for (int i = 0; i < size; i++) {
		delete heightMap[i];
	}
	delete heightMap;
	generateHeightMap();
}

void HeightMap::squareStep(int chunkSize, int halfChunkSize)
{
	if (width != length)
		throw std::exception("Width and Lenght of the heightmap do not match");

	int size = width;
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
	if (width != length)
		throw std::exception("Width and Lenght of the heightmap do not match");

	int size = width;
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
