#include "skybox.h"

#include <external/stb_image.h>

Skybox::Skybox(std::vector<std::string> faceLocations)
{
	skyboxShader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, bitDepth;

	for (int i = 0; i < faceLocations.size(); i++)
	{
		stbi_set_flip_vertically_on_load(0);
		unsigned char* textureData = stbi_load(faceLocations[i].c_str(), &width, &height, &bitDepth, 0);
		if (!textureData)
		{
			printf("Failed to load the texture at %s\n", faceLocations[i].c_str());
			return;
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

		stbi_image_free(textureData);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Vertex vertices[] = {

		Vertex(glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 0
		Vertex(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 1
		Vertex(glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 2
		Vertex(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 3

		Vertex(glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 4
		Vertex(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 5
		Vertex(glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 6
		Vertex(glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec2(0.0f,0.0f)), // 7
		//-1.0f,  1.0f, -1.0f, // 0
		//-1.0f, -1.0f, -1.0f, // 1
		// 1.0f, -1.0f, -1.0f, // 2
		// 1.0f,  1.0f, -1.0f, // 3

		//-1.0f,  1.0f,  1.0f, // 4
		//-1.0f, -1.0f,  1.0f, // 5
		// 1.0f, -1.0f,  1.0f, // 6
		// 1.0f,  1.0f,  1.0f, // 7
	};

	unsigned int indices[] = {
		// front
	0, 1, 2,
	2, 1, 3,
	// right
	2, 3, 5,
	5, 3, 7,
	// back
	5, 7, 4,
	4, 7, 6,
	// left
	4, 6, 0,
	0, 6, 1,
	// top
	4, 0, 5,
	5, 0, 2,
	// bottom
	1, 6, 3,
	3, 6, 7
	};

	skyboxMesh = new Mesh(0, vertices, 8, indices, 36, *skyboxShader);
}

Skybox::~Skybox()
{
}

void Skybox::DrawSkybox(glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	viewMatrix = glm::mat4(glm::mat3(viewMatrix));

	glDepthMask(GL_FALSE);

	skyboxShader->use();

	skyboxShader->setMat4("projection", projectionMatrix);
	skyboxShader->setMat4("view", viewMatrix);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	skyboxMesh->draw();

	glDepthMask(GL_TRUE);
}
