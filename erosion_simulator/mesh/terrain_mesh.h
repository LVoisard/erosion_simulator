#pragma once
#include "mesh.h"

class TerrainMesh :  public Mesh
{
public:
	TerrainMesh(int size, double*** terrainHeights, Shader shader);
	TerrainMesh(int size, HeightMap* heightMap, Shader shader);
	~TerrainMesh();	
private:
};

