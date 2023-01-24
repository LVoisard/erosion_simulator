#pragma once
#include <cstdint>
#include <string>

#include "glad/glad.h"

#include "glm/glm.hpp"

class Shader
{
public:
	Shader(const char* vertexShaderFilePath, const char* fragmentShaderFilePath);
	~Shader();

	void use() { glUseProgram(ID); }
	void stop() { glUseProgram(0); }
	void clear();

	void setMat4(const char* name, glm::mat4& matrix);
	void setTexture(const char* name, int activeTexture);
	void setTextures(const char* name, int texCount, int* values);

	uint32_t ID;
private:
	
	std::string readFile(const char* filePath);
	void addShader(const char* shaderCode, int shaderType);
};
