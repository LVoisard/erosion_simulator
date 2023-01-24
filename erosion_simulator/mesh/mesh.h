#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "height_map/height_map.h"

struct Vertex 
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

class Mesh
{
public:
	Mesh();
	Mesh(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);
	~Mesh();
	void init();
	void draw();

	void createSquareMesh(int size, double** heights);

	Vertex* vertices;
	uint32_t vertexCount = 0;

	uint32_t* indices;
	uint32_t indexCount = 0;
protected:
	void update();
	void clearData();
private:

	uint32_t VAO, VBO, EBO;
};

