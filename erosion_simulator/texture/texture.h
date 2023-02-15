#pragma once
#include <cstdint>

class Texture
{
public:
	Texture(const char* textureFilePath);
	Texture(const unsigned int width, const unsigned int height);

	void use();
	void use(int activeTexture);
	void clear();

private:
	void load();

	uint32_t ID;
	int width, height, bitDepth;

	const char* textureFilePath;
};

