#include "camera.h"

#include <iostream>

Camera::Camera(Window* window, float speed, float xSensitivity, float ySensitivity)
	:window(window), speed(speed), xSensitivity(xSensitivity), ySensitivity(ySensitivity) {}

void Camera::update(float dt)
{
	if (!inFreeview) return;

	if (window->getKey(GLFW_KEY_W))
		position += speed * dt * lookAt;
	if (window->getKey(GLFW_KEY_S))
		position -= speed * dt * lookAt;
	if (window->getKey(GLFW_KEY_A))
		position -= speed * dt * right;
	if (window->getKey(GLFW_KEY_D))
		position += speed * dt * right;
	if (window->getKey(GLFW_KEY_Q))
		position -= speed * dt * worldUp;
	if (window->getKey(GLFW_KEY_E))
		position += speed * dt * worldUp;
	
	speed += window->getMouseScrollY();
	if (speed < 1)
		speed = 1;

	yaw += window->getMouseDeltaX() * xSensitivity;
	pitch += window->getMouseDeltaY() * ySensitivity;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	lookAt.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	lookAt.y = sin(glm::radians(pitch));
	lookAt.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	lookAt = glm::normalize(lookAt);
	
	right = glm::normalize(glm::cross(lookAt, worldUp));
	up = glm::normalize(glm::cross(right, lookAt));
	forward = glm::normalize(glm::cross(worldUp, right));
}
