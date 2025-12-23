#pragma once

struct Transform;

namespace Math
{
	glm::mat4 CalculateWorldMatrix(const Transform& transform);
	glm::mat4 CalculateViewMatrix(const glm::vec3& viewPosition, const glm::vec3& viewRotation);
	glm::mat4 CalculatePerspectiveProjectionMatrix(const float fov, const float width, const float height, const float nearClipPlane, const float farClipPlane);
	glm::mat4 CalculateOrthographicProjectionMatrix(const float width, const float height, const float nearClipPlane, const float farClipPlane);
	glm::vec3 RotateVector(const glm::vec3& rotation, const glm::vec3& vector);
	glm::vec3 FindLookAtRotation(const glm::vec3& currentPosition, const glm::vec3& targetPosition, const glm::vec3& up);
}