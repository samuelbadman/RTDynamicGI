#pragma once

namespace Renderer
{
	struct Camera
	{
		struct CameraSettings
		{
			enum class ProjectionMode : uint8_t
			{
				PERSPECTIVE,
				ORTHOGRAPHIC,
			};

			ProjectionMode ProjectionMode = ProjectionMode::PERSPECTIVE;

			// Orthographic settings
			float OrthographicWidth = 1.6f;
			float OrthographicHeight = 0.9f;
			float OrthographicNearClipPlane = 0.0f;
			float OrthographicFarClipPlane = 10000.0f;

			// Perspective settings
			// Field of view in degrees
			float PerspectiveFOV = 90.0f;
			float PerspectiveNearClipPlane = 0.01f;
			float PerspectiveFarClipPlane = 100.0f;
		};

		glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		CameraSettings Settings = {};
	};
}