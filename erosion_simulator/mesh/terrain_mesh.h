#pragma once
#include "mesh.h"

class TerrainMesh :  public Mesh
{
public:
	TerrainMesh(int size, HeightMap* heightMap);
	~TerrainMesh();

	void updateMeshFromMap();
private:
	void calculateMeshFromMap();
	void recalculateNormals();

	HeightMap* heightMap;
	int size;
};

