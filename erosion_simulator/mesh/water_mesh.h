#pragma once
#include "terrain_mesh.h"
#include "shader/shader.h"

class WaterMesh : public Mesh
{
public:
	WaterMesh(int width, int length, float*** waterFloor, float*** waterHeight, Shader shader);

	void updateMeshFromHeights(float*** waterFloor, float*** waterHeight, glm::vec2*** waterVelocities, float*** sediment);
	void changeVerticesWaterHeight(float*** waterHeight);
	void changeVerticesWaterVelocities(glm::vec2*** waterVelocities);
	void changeVerticesWaterSediment(float*** sediments);
	virtual void init() override;
	virtual void calculateNormals() override;
private:
};

