#include "water_mesh.h"
#include "glad/glad.h"
#include "shader/shader.h"
#include "mesh.h"

WaterMesh::WaterMesh(int width, int length, float*** waterFloor, float*** waterHeight, Shader shader)
	:Mesh(width, length, shader)
{	
	calculateVertices(waterFloor);
	changeVerticesWaterHeight(waterHeight);
	calculateIndices();
	calculateNormals();
}

void WaterMesh::updateMeshFromHeights(float*** waterFloor, float*** waterHeight, glm::vec2*** waterVelocities, float*** sediments)
{
	clearData();
	calculateVertices(waterFloor);
	changeVerticesWaterHeight(waterHeight);
	changeVerticesWaterVelocities(waterVelocities);
	changeVerticesWaterSediment(sediments);
	calculateIndices();
	calculateNormals();
	update();
}

void WaterMesh::changeVerticesWaterHeight(float*** waterHeight)
{
	for (int y = 0; y < length; y++)
	{
		for (int x = 0; x < width; x++)
		{
			vertices[y * length + x].height = (*waterHeight)[x][y];
		}
	}
}

void WaterMesh::changeVerticesWaterVelocities(glm::vec2*** waterVelocities)
{
	for (int y = 0; y < length; y++)
	{
		for (int x = 0; x < width; x++)
		{
			vertices[y * length + x].velocity = (*waterVelocities)[x][y];
		}
	}
}

void WaterMesh::changeVerticesWaterSediment(float*** sediments)
{
	for (int y = 0; y < length; y++)
	{
		for (int x = 0; x < width; x++)
		{
			vertices[y * length + x].currentSediment = (*sediments)[x][y];
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
	glEnableVertexAttribArray(shader.getAttribLocation("sediment"));
	glVertexAttribPointer(shader.getAttribLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(0));
	glVertexAttribPointer(shader.getAttribLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos)));
	glVertexAttribPointer(shader.getAttribLocation("uv"), 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal)));
	glVertexAttribPointer(shader.getAttribLocation("height"), 1, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal) + sizeof(vertices[0].uv)));
	glVertexAttribPointer(shader.getAttribLocation("velocity"), 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal) + sizeof(vertices[0].uv) + sizeof(vertices[0].height)));
	glVertexAttribPointer(shader.getAttribLocation("sediment"), 1, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal) + sizeof(vertices[0].uv) + sizeof(vertices[0].height) + sizeof(vertices[0].velocity)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void WaterMesh::calculateNormals()
{
	for (int y = 1; y < (length - 1); y++)
	{
		for (int x = 1; x < (width - 1); x++)
		{
			glm::vec3 center = vertices[y * length + x].pos;

			glm::vec3 top = y == length - 1 ? glm::vec3(0) : vertices[(y + 1) * length + x].pos + vertices[(y + 1) * length + x].height * glm::vec3(0,1,0);
			glm::vec3 bottom = y == 0 ? glm::vec3(0) : vertices[(y - 1) * length + x].pos + vertices[(y - 1) * length + x].height * glm::vec3(0, 1, 0);
			glm::vec3 right = x == width - 1 ? glm::vec3(0) : vertices[y * length + x + 1].pos + vertices[y * length + x + 1].height * glm::vec3(0, 1, 0);
			glm::vec3 left = x == 0 ? glm::vec3(0) : vertices[y * length + x - 1].pos + vertices[y * length + x - 1].height * glm::vec3(0, 1, 0);

			glm::vec3 v1 = normalize(right - center);
			glm::vec3 v2 = normalize(top - center);
			glm::vec3 v3 = normalize(left - center);
			glm::vec3 v4 = normalize(bottom - center);

			glm::vec3 normal1 = cross(v2, v1);
			glm::vec3 normal2 = cross(v3, v2);
			glm::vec3 normal3 = cross(v4, v3);
			glm::vec3 normal4 = cross(v1, v4);

			glm::vec3 normal = glm::vec3(0);

			if (x == 0 || x == width - 1 || y == 0 || y == length - 1)
			{
				if (top == glm::vec3(0))
				{
					if (left != glm::vec3(0))
						normal += normal3;
					if (right != glm::vec3(0))
						normal += normal4;
				}
				else if (bottom == glm::vec3(0))
				{
					if (left != glm::vec3(0))
						normal += normal2;
					if (right != glm::vec3(0))
						normal += normal1;
				}

				if (left == glm::vec3(0))
				{
					if (top != glm::vec3(0))
						normal += normal1;
					if (bottom != glm::vec3(0))
						normal += normal4;
				}
				else if (right == glm::vec3(0))
				{
					if (top != glm::vec3(0))
						normal += normal2;
					if (bottom != glm::vec3(0))
						normal += normal3;
				}
			}
			else
			{
				normal = normal1 + normal2 + normal3 + normal4;
			}

			vertices[y * length + x].normal = glm::normalize(normal);
		}
	}
}
