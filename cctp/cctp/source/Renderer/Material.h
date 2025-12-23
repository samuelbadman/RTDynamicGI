#pragma once

namespace Renderer
{
	class Material
	{
	public:
		const glm::vec4& GetColor() const { return Color; }
		void SetColor(const glm::vec4& color) { Color = color; }

	private:
		glm::vec4 Color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	};
}