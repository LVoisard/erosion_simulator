#pragma once
#include "terrain_mesh.h"
#include "shader/shader.h"

class WaterMesh : public Mesh
{
public:
	WaterMesh(int size, double** waterFloor, double** waterHeight, Shader shader);

	void changeVerticesWaterHeight(double** waterHeight);
	virtual void init() override;
private:
};

