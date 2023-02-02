#pragma once
#include "mesh.h"

class TerrainMesh :  public Mesh
{
public:
	TerrainMesh(int size, float*** terrainHeights, Shader shader);
	TerrainMesh(int size, HeightMap* heightMap, Shader shader);
	~TerrainMesh();	

	virtual void updateMeshFromHeights(float*** heights) override;
	void updateOriginalHeights();
	void updateOriginalHeights(float*** heights);
	virtual void init() override;

	glm::vec3 getNormalAtPosition(int x, int y);
private:

	float* originalHeights;
};

