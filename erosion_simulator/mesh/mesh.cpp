#include "mesh.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Mesh::Mesh()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &VBO);
}

Mesh::Mesh(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount):
	vertices(vertices), vertexCount(vertexCount), indices(indices), indexCount(indexCount)
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

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (const GLvoid*)(sizeof(vertices[0].pos) + sizeof(vertices[0].normal)));

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

void Mesh::createSquareMesh(int size, double** heights)
{
	size = size;
	vertexCount = size * size;
	vertices = new Vertex[vertexCount];

	for (int z = 0; z < size; z++) {
		for (int x = 0; x < size; x++) {
			Vertex v{};
			v.pos = glm::vec3(x - size / 2, heights[x][z], z - size / 2) * (100.0f / size);
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

	init();
}

void Mesh::update()
{	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(this->vertices[0]) * vertexCount, this->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	// memory leak on gpu ??, usage goes from 5% to 100%
	/*glBindBuffer(GL_ARRAY_BUFFER, VBO);	
	void* data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(data, vertices, sizeof(vertices[0]) * vertexCount);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, 0);*/
}

void Mesh::clearData()
{
	delete vertices; 
	vertexCount = 0;
	delete indices; 
	indexCount = 0;
}
