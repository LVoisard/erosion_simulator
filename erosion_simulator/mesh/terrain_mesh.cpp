#include "terrain_mesh.h"

TerrainMesh::TerrainMesh(int size, double** terrainHeights, Shader shader)
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

void TerrainMesh::updateMeshFromMap(HeightMap* heightMap)
{
	clearData();
	calculateVertices(heightMap);
	update();
}

void TerrainMesh::updateMeshFromHeights(double** heights)
{
	clearData();
	calculateVertices(heights);
	update();
}
