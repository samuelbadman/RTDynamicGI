#include "Pch.h"
#include "Math.h"
#include "Transform.h"

glm::mat4 Math::CalculateWorldMatrix(const Transform& transform)
{
	glm::vec3 eulerAnglesRadians = { glm::radians(transform.Rotation.x),
	glm::radians(transform.Rotation.y),
	glm::radians(transform.Rotation.z) };
	glm::quat rotationQuaternion = glm::quat(eulerAnglesRadians);

	return glm::translate(glm::identity<glm::mat4>(), transform.Position) *  // Translation matrix
		glm::mat4_cast(rotationQuaternion) * // Rotation matrix
		glm::scale(glm::identity<glm::mat4>(), transform.Scale); // Scale matrix
}

glm::mat4 Math::CalculateViewMatrix(const glm::vec3& viewPosition, const glm::vec3& viewRotation)
{
	glm::vec3 eulerAnglesRadians = { glm::radians(viewRotation.x),
		glm::radians(viewRotation.y),
		glm::radians(viewRotation.z) };
	glm::quat rotationQuaternion = glm::quat(eulerAnglesRadians);

	return glm::inverse(glm::translate(glm::identity<glm::mat4>(), viewPosition) *
		glm::mat4_cast(rotationQuaternion));
}

glm::mat4 Math::CalculatePerspectiveProjectionMatrix(const float fov, const float width, const float height, const float nearClipPlane, const float farClipPlane)
{
	return glm::perspectiveFovLH(glm::radians(fov), width, height, nearClipPlane, farClipPlane);
}

glm::mat4 Math::CalculateOrthographicProjectionMatrix(const float width, const float height, const float nearClipPlane, const float farClipPlane)
{
	return glm::orthoLH(-width, width, -height, height, nearClipPlane, farClipPlane);
}

glm::vec3 Math::RotateVector(const glm::vec3& rotation, const glm::vec3& vector)
{
	glm::vec3 eulerRotationRadians = {
	glm::radians(rotation.x),
	glm::radians(rotation.y),
	glm::radians(rotation.z)
	};

	glm::quat rotationQuaternion = glm::quat(eulerRotationRadians);
	glm::mat3 rotationMatrix = glm::mat3_cast(rotationQuaternion);

	// Rotate the vector to match the rotation
	return rotationMatrix * vector;
}

glm::vec3 Math::FindLookAtRotation(const glm::vec3& currentPosition, const glm::vec3& targetPosition, const glm::vec3& up)
{
	glm::vec3 eulerRotation;
	glm::extractEulerAngleXYZ(glm::lookAt(currentPosition, targetPosition, up), eulerRotation.x, eulerRotation.y, eulerRotation.z);
	eulerRotation.x = glm::degrees(eulerRotation.x);
	eulerRotation.y = glm::degrees(eulerRotation.y);
	eulerRotation.z = glm::degrees(eulerRotation.z);
	return eulerRotation;
}

