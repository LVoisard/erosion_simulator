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
	float waterHeight;
};

class Mesh
{
public:
	Mesh(int size, Shader shader);
	Mesh(int size, Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount, Shader shader);
	~Mesh();
	virtual void init();
	void draw();
	void updateMeshFromMap(HeightMap* heightMap);
	void updateMeshFromHeights(double*** heights);

	Vertex* vertices;
	uint32_t vertexCount = 0;

	uint32_t* indices;
	uint32_t indexCount = 0;
	Shader shader;
protected:
	int size;
	virtual void calculateVertices(HeightMap* map);
	virtual void calculateVertices(double*** height);
	virtual void calculateIndices();
	virtual void calculateNormals();


	void update();
	void clearData();
	uint32_t VAO, VBO, EBO;
private:

};

