#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "height_map/height_map.h"
#include "shader/shader.h"

struct Vertex 
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	float height;
	glm::vec2 velocity;
};

class Mesh
{
public:
	Mesh(int width, int length, Shader shader);
	Mesh(int width, int length, Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount, Shader shader);
	~Mesh();
	virtual void init();
	void draw();
	void updateMeshFromMap(HeightMap* heightMap);
	virtual void updateMeshFromHeights(float*** heights);

	Vertex* vertices;
	uint32_t vertexCount = 0;

	uint32_t* indices;
	uint32_t indexCount = 0;
	Shader shader;
protected:
	int width, length;
	virtual void calculateVertices(HeightMap* map);
	virtual void calculateVertices(float*** height);
	virtual void calculateIndices();
	virtual void calculateNormals();


	void update();
	void clearData();
	uint32_t VAO, VBO, EBO;
private:

};

