#pragma once
#include "terrain_mesh.h"
#include "shader/shader.h"

class WaterMesh : public Mesh
{
public:
	WaterMesh(int size, double*** waterFloor, double*** waterHeight, Shader shader);

	void updateMeshFromHeights(double*** waterFloor, double*** waterHeight);
	void changeVerticesWaterHeight(double*** waterHeight);
	virtual void init() override;
private:
};

