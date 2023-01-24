#pragma once
#include <vector>
#include <string>
#include <mesh/mesh.h>
#include <shader/shader.h>
#include <glm/glm.hpp>

class Skybox
{
public:
	Skybox(std::vector<std::string> faceLocations);
	~Skybox();

	void DrawSkybox(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

private:
	Mesh* skyboxMesh;
	Shader* skyboxShader;

	uint32_t textureID;
};

