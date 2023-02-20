#include "mesh.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Mesh::Mesh(int width, int length, Shader shader)
	: width(width), length(length), shader(shader)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &VBO);
}

Mesh::Mesh(int width, int length, Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount, Shader shader)
	: width(width), length(length), vertices(vertices), vertexCount(vertexCount), indices(indices), indexCount(indexCount), shader(shader)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &VBO);

	init();
}

Mesh::~Mesh()
{
	clearData();

	if (EBO != 0)
	{
		glDeleteBuffers(1, &EBO);
		EBO = 0;
	}

	if (VBO != 0)
	{
		glDeleteBuffers(1, &VBO);
		VBO = 0;
	}

	if (VAO != 0)
	{
		glDeleteVertexArrays(1, &VAO);
		VAO = 0;
	}
}

void Mesh::init()
{
	glBindVertexArray(VAO);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indexCount, indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertexCount, vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.getAttribLocation("pos"));
	glEnableVertexAttribArray(shader.getAttribLocation("normal"));
	glEnableVertexAttribArray(shader.getAttribLocation("uv"));
	glVertexAttribPointer(shader.getAttribLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(0));
	glVertexAttribPointer(shader.getAttribLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos)));
	glVertexAttribPointer(shader.getAttribLocation("uv"), 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::draw()
{
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::calculateVertices(HeightMap* map)
{
	vertexCount = width * length;
	vertices = new Vertex[vertexCount];

	for (int z = 0; z < length; z++) {
		for (int x = 0; x < width; x++) {
			Vertex v{};
			v.pos = glm::vec3(x - width / 2, map->samplePoint(x, z), z - length / 2);
			v.normal = v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			v.uv = glm::vec2((float)x / width, (float)z / length) / (10.0f / width);
			vertices[z * length + x] = v;
		}
	}
}

void Mesh::calculateVertices(float*** height)
{
	vertexCount = width * length;
	vertices = new Vertex[vertexCount];

	for (int z = 0; z < length; z++) {
		for (int x = 0; x < width; x++) {
			Vertex v{};
			v.pos = glm::vec3(x - width / 2, (*height)[x][z], z - length / 2);
			v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			v.uv = glm::vec2((float)x / width, (float)z / length) / (10.0f / width);
			vertices[z * length + x] = v;
		}
	}
}

void Mesh::calculateIndices()
{
	indexCount = (width - 1) * (length - 1) * 6;
	indices = new uint32_t[indexCount];
	int indicesIndex = 0;
	for (int z = 0; z < (length - 1); z++) {
		for (int x = 0; x < (width - 1); x++) {
			indices[indicesIndex + z * (length - 1) + x] = z * length + x; // 0
			indices[indicesIndex + z * (length - 1) + x + 1] = (z + 1) * length + x; // 2
			indices[indicesIndex + z * (length - 1) + x + 2] = z * length + x + 1; // 1

			// top triangle
			indices[indicesIndex + z * (length - 1) + x + 3] = (z + 1) * length + x + 1; // 3
			indices[indicesIndex + z * (length - 1) + x + 4] = z * length + x + 1; // 1
			indices[indicesIndex + z * (length - 1) + x + 5] = (z + 1) * length + x; // 2

			indicesIndex += 5;
		}
	}
}

void Mesh::calculateNormals()
{
	for (int y = 0; y < length; y++)
	{
		for (int x = 0; x < width; x++)
		{
			glm::vec3 center = vertices[y * length + x].pos;

			glm::vec3 top = y == length - 1 ? glm::vec3(0) : vertices[(y + 1) * length + x].pos;
			glm::vec3 bottom = y == 0 ? glm::vec3(0) : vertices[(y - 1) * length + x].pos;
			glm::vec3 right = x == width - 1 ? glm::vec3(0) : vertices[y * length + x + 1].pos;
			glm::vec3 left = x == 0 ? glm::vec3(0) : vertices[y * length + x - 1].pos;

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

void Mesh::updateMeshFromMap(HeightMap* heightMap)
{
	clearData();
	calculateVertices(heightMap);
	calculateIndices();
	calculateNormals();
	update();
}

void Mesh::updateMeshFromHeights(float*** heights)
{
	clearData();
	calculateVertices(heights);
	calculateIndices();
	calculateNormals();
	update();
}

void Mesh::update()
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);	
		void* data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(data, vertices, sizeof(vertices[0]) * vertexCount);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::clearData()
{
	delete vertices; 
	vertexCount = 0;
	delete indices; 
	indexCount = 0;
}
