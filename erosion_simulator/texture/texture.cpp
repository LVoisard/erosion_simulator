#include "texture.h"
#include "glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#include <string>

Texture::Texture(const char* textureFilePath)
	:ID(0), width(0), height(0), bitDepth(0), textureFilePath(textureFilePath)
{
	load();
}

void Texture::use()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::use(int activeTexture)
{
	glActiveTexture(activeTexture);
	glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::clear()
{
	glDeleteTextures(1, &ID);
	ID = 0;
	width = 0;
	height = 0;
	bitDepth = 0;
	delete textureFilePath;
}

void Texture::load()
{
	stbi_set_flip_vertically_on_load(1);
	unsigned char* textureData = stbi_load(textureFilePath, &width, &height, &bitDepth, 0);
	if (!textureData)
	{
		printf("Failed to load the texture at %s\n", textureFilePath);
		return;
	}

	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D, ID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(std::string(textureFilePath).ends_with(".png"))
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
	else if(std::string(textureFilePath).ends_with(".jpg"))
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(textureData);
}
