#include "terrain_mesh.h"

TerrainMesh::TerrainMesh(int size, HeightMap* heightMap)
	:size(size), heightMap(heightMap), Mesh()
{
	calculateMeshFromMap();
	init();
}

TerrainMesh::~TerrainMesh()
{
}

void TerrainMesh::updateMeshFromMap()
{
	clearData();
	calculateMeshFromMap();
	update();
}

void TerrainMesh::calculateMeshFromMap()
{
	vertexCount = size * size;
	vertices = new Vertex[vertexCount];

	for (int z = 0; z < size; z++) {
		for (int x = 0; x < size; x++) {
			Vertex v{};
			v.pos = glm::vec3(x - size / 2, heightMap->samplePoint(x, z), z - size / 2) * (100.0f / size);
			v.normal = glm::vec3(0.0f);
			v.uv = glm::vec2((float)x / size, (float)z / size) / (10.0f / size);
			vertices[z * size + x] = v;
		}
	}

	indexCount = (size - 1) * (size - 1) * 6;
	indices = new uint32_t[indexCount];
	int indicesIndex = 0;
	for (int z = 0; z < (size - 1); z++) {
		for (int x = 0; x < (size - 1); x++) {
			indices[indicesIndex + z * (size - 1) + x] = z * size + x; // 0
			indices[indicesIndex + z * (size - 1) + x + 1] = (z + 1) * size + x; // 2
			indices[indicesIndex + z * (size - 1) + x + 2] = z * size + x + 1; // 1

			// top triangle
			indices[indicesIndex + z * (size - 1) + x + 3] = (z + 1) * size + x + 1; // 3
			indices[indicesIndex + z * (size - 1) + x + 4] = z * size + x + 1; // 1
			indices[indicesIndex + z * (size - 1) + x + 5] = (z + 1) * size + x; // 2

			indicesIndex += 5;
		}
	}

	// recalculateNormals();
}

void TerrainMesh::recalculateNormals()
{
	for (int y = 1; y < (size - 1); y++)
	{
		for (int x = 1; x < (size - 1); x++)
		{
			glm::vec3 center = vertices[y * size + x].pos;

			glm::vec3 right = vertices[y * size + x + 1].pos;
			glm::vec3 up = vertices[(y + 1) * size + x].pos;

			glm::vec3 left = vertices[y * size + x - 1].pos;
			glm::vec3 bottom = vertices[(y - 1) * size + x].pos;


			glm::vec3 v1 = normalize(right - center);
			glm::vec3 v2 = normalize(up - center);
			glm::vec3 v3 = normalize(left - center);
			glm::vec3 v4 = normalize(bottom - center);

			glm::vec3 normal1 = cross(v2, v1);
			glm::vec3 normal2 = cross(v3, v2);
			glm::vec3 normal3 = cross(v4, v3);
			glm::vec3 normal4 = cross(v1, v4);

			glm::vec3 normal = normal1 + normal2 + normal3 + normal4;

			// ridge check
			/*if (normal.y < 2)
			{
				glm::vec3 norms[] = {normal1, normal2, normal3, normal4};
				glm::vec3 smallestYNorm = norms[0];
				for (int i = 0; i < 4; i++)
				{
					if (norms[i].y < smallestYNorm.y)
						smallestYNorm = norms[i];
				}
				normal = smallestYNorm;
			}*/

			vertices[y * size + x].normal = glm::normalize(normal);
		}
	}
}
