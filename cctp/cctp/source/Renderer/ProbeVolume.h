#pragma once

#include "Math/Transform.h"

namespace Renderer
{
	class ProbeVolume
	{
	public:
		ProbeVolume(const glm::vec3& position, const glm::vec3& volumeExtents, float probeSpacing, float debugProbeSize);
		void Update();

		const auto& GetProbeTransforms() const { return ProbeTransforms; }
		auto& GetVolumePosition() { return Position; } // Returns the center of the probe volume in world space
		auto GetTotalProbeCount() const { return ProbeCountX * ProbeCountY * ProbeCountZ; }
		const auto& GetProbeCountX() const { return ProbeCountX; }
		const auto& GetProbeCountY() const { return ProbeCountY; }
		const auto& GetProbeCountZ() const { return ProbeCountZ; }
		const auto& GetProbeSpacing() const { return ProbeSpacing; }

	private:
		void UpdateProbePositions();

	private:
		glm::vec3 Position;
		glm::vec3 Extents;
		float ProbeSpacing;
		std::vector<Transform> ProbeTransforms;
		glm::vec3 UpdatedPosition;
		size_t ProbeCountX;
		size_t ProbeCountY;
		size_t ProbeCountZ;
	};
}
