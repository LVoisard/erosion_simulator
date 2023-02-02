#include "water_mesh.h"
#include "glad/glad.h"
#include "shader/shader.h"
#include "mesh.h"

WaterMesh::WaterMesh(int size, float*** waterFloor, float*** waterHeight, Shader shader)
	:Mesh(size, shader)
{	
	calculateVertices(waterFloor);
	changeVerticesWaterHeight(waterHeight);
	calculateIndices();
	calculateNormals();
}

void WaterMesh::updateMeshFromHeights(float*** waterFloor, float*** waterHeight, glm::vec2*** waterVelocities)
{
	clearData();
	calculateVertices(waterFloor);
	changeVerticesWaterHeight(waterHeight);
	changeVerticesWaterVelocities(waterVelocities);
	calculateIndices();
	calculateNormals();
	update();
}

void WaterMesh::changeVerticesWaterHeight(float*** waterHeight)
{
	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			vertices[y * size + x].height = (*waterHeight)[x][y];
		}
	}
}

void WaterMesh::changeVerticesWaterVelocities(glm::vec2*** waterVelocities)
{
	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			vertices[y * size + x].velocity = (*waterVelocities)[x][y];
		}
	}
}

void WaterMesh::init()
{
	glBindVertexArray(VAO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indexCount, indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertexCount, vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.getAttribLocation("pos"));
	glEnableVertexAttribArray(shader.getAttribLocation("normal"));
	glEnableVertexAttribArray(shader.getAttribLocation("uv"));
	glEnableVertexAttribArray(shader.getAttribLocation("height"));
	glEnableVertexAttribArray(shader.getAttribLocation("velocity"));
	glVertexAttribPointer(shader.getAttribLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(0));
	glVertexAttribPointer(shader.getAttribLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos)));
	glVertexAttribPointer(shader.getAttribLocation("uv"), 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal)));
	glVertexAttribPointer(shader.getAttribLocation("height"), 1, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal) + sizeof(vertices[0].uv)));
	glVertexAttribPointer(shader.getAttribLocation("velocity"), 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal) + sizeof(vertices[0].uv) + sizeof(vertices[0].height)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void WaterMesh::calculateNormals()
{
	for (int y = 1; y < (size - 1); y++)
	{
		for (int x = 1; x < (size - 1); x++)
		{
			glm::vec3 center = vertices[y * size + x].pos + vertices[y * size + x].height;

			glm::vec3 right = vertices[y * size + x + 1].pos + vertices[y * size + x + 1].height;
			glm::vec3 up = vertices[(y + 1) * size + x].pos + vertices[(y + 1) * size + x].height;

			glm::vec3 left = vertices[y * size + x - 1].pos + vertices[y * size + x - 1].height;
			glm::vec3 bottom = vertices[(y - 1) * size + x].pos + vertices[(y - 1) * size + x].height;


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
