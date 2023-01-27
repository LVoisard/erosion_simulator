#include "terrain_mesh.h"

TerrainMesh::TerrainMesh(int size, double*** terrainHeights, Shader shader)
	:Mesh(size, shader)
{
	calculateVertices(terrainHeights);
	calculateIndices();
	calculateNormals();
}

TerrainMesh::TerrainMesh(int size, HeightMap* heightMap, Shader shader)
	:Mesh(size, shader)
{
	calculateVertices(heightMap); 
	calculateIndices();
	calculateNormals();
}

TerrainMesh::~TerrainMesh()
{
}
