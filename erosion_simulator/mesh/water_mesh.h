#pragma once
#include "terrain_mesh.h"
#include "shader/shader.h"

class WaterMesh : public Mesh
{
public:
	WaterMesh(int size, float*** waterFloor, float*** waterHeight, Shader shader);

	void updateMeshFromHeights(float*** waterFloor, float*** waterHeight, glm::vec2*** waterVelocities);
	void changeVerticesWaterHeight(float*** waterHeight);
	void changeVerticesWaterVelocities(glm::vec2*** waterVelocities);
	virtual void init() override;
	virtual void calculateNormals() override;
private:
};

