#pragma once

struct Transform
{
	glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
	// Pitch, yaw, roll rotation, expressed in degrees
	glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
	glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };
};