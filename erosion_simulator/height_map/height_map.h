#pragma once
#include <random>
#include <string>

class HeightMap
{
public:
	HeightMap(double minHeight, double maxHeight);
	void createProceduralHeightMap(int size, double random);
	void loadHeightMapFromFile(std::string);
	void loadHeightMapFromOBJFile(std::string, float heightDiff);
	void setHeightRange(double minHeight, double maxHeight);
	void setRandomRange(double random);
	double** getHeightMap() { return heightMap; }
	int getWidth() { return width; }
	int getLength() { return length; }
	void printMap();
	void changeSeed() {mapGenerator.seed(seedDistr(seedGenerator)); regenerateHeightMap(); }
	void saveHeightMapPPM(std::string fileName);
	void saveHeightMapPPM(std::string fileName, float*** hmp);
	double samplePoint(int x, int y) { return heightMap[x][y]; }
	double getRGBA(int x, int y) { return std::clamp((double)heightMap[x][y] + minHeight / (double)maxHeight + minHeight, 0.0, 1.0); }
	int getMaxHeight() { return maxHeight; }


private:
	void init();
	void generateHeightMap();
	void regenerateHeightMap();
	void squareStep(int chunkSize, int halfChunkSize);
	void diamondStep(int chunkSize, int halfChunkSize);

	double** heightMap;

	int width;
	int length;

	double minHeight;
	double maxHeight;

	double random;

	std::default_random_engine seedGenerator;
	std::uniform_int_distribution<int> seedDistr;

	std::default_random_engine mapGenerator;
	std::uniform_real_distribution<> heightDistr;
	std::uniform_real_distribution<> randomDistr;
};

