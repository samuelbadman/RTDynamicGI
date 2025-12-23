#pragma once

namespace Renderer
{
	struct Vertex1Pos1UV1Norm
	{
		glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec2 UV = glm::vec2(0.0f, 0.0f);
		glm::vec3 Normal = glm::vec3(0.0f, 0.0f, 0.0f);
	};
}