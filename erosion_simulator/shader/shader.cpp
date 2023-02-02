#include "shader.h"

#include <fstream>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
{
    ID = glCreateProgram();

    if (!ID)
    {
        printf("Error creating the shader");
        return;
    }

    std::string vertexCode = readFile(vertexShaderFilePath);
    std::string fragmentCode = readFile(fragmentShaderFilePath);

    addShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    addShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
    
    int success = 0;
    char infoLog[1024];

    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 1024, NULL, infoLog);
        printf("Error liking program: '%s'\n", infoLog);
        return;
    }

    glValidateProgram(ID);
    glGetProgramiv(ID, GL_VALIDATE_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, sizeof(infoLog), NULL, infoLog);
        printf("Error validating program: '%s'\n", infoLog);
        return;
    }
}

Shader::~Shader()
{
    if (ID != 0)
        glDeleteProgram(0);
}

void Shader::setUniformVector3(const char* name, glm::vec3 vec)
{
    glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(vec));

}

void Shader::setUniformFloat(const char* name, float* val)
{
    glUniform1fv(glGetUniformLocation(ID, name), 1, val);
}

void Shader::setMat4(const char* name, glm::mat4& matrix)
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::setTexture(const char* name, int activeTexture)
{
    glUniform1i(glGetUniformLocation(ID, name), activeTexture);
}

void Shader::setTextures(const char* name, int texCount, int* values)
{
    glUniform1iv(glGetUniformLocation(ID, name), texCount, values);
}

std::string Shader::readFile(const char* filePath)
{
	std::string code;
	std::ifstream fileStream(filePath, std::ios::in);

	if (!fileStream.is_open())
	{
		printf("Failed to read the file %s, file does not exist", filePath);
		return "";
	}

	std::string line = "";
	while (!fileStream.eof())
	{
		std::getline(fileStream, line);
		code.append(line + "\n");
	}

	fileStream.close();
	return code;
}

void Shader::addShader(const char* shaderCode, int shaderType)
{
    const GLchar* code[1];
    code[0] = shaderCode;

    GLint codeLength[1];
    codeLength[0] = strlen(shaderCode);

    uint32_t shaderID = glCreateShader(shaderType);
    glShaderSource(shaderID, 1, code, NULL);
    glCompileShader(shaderID);
    // check for shader compile errors
    
    int success;
    char infoLog[1024];

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shaderID, 1024, NULL, infoLog);
        printf("Error compiling the %d shader: '%s'\n", shaderType, infoLog);
        return;
    }

    glAttachShader(ID, shaderID);
    glDeleteShader(shaderID);
}
