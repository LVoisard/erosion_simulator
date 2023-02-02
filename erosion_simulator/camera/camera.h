#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "window/window.h"

class Camera
{
public:
	Camera(Window* window, float speed, float xSensitivity, float ySensitivity);
	void update(float dt);
	glm::vec3 getPosition() { return position; }

	glm::mat4 getViewMatrix() { return glm::lookAt(position, position + lookAt, worldUp); }
private:
	Window* window;

	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 right = glm::normalize(glm::cross(lookAt, worldUp));
	glm::vec3 up = glm::normalize(glm::cross(right, lookAt));
	glm::vec3 forward = glm::cross(worldUp, right);

	float yaw;
	float pitch;

	float speed = 5;
	float xSensitivity;
	float ySensitivity;

	bool inFreeview;
};

