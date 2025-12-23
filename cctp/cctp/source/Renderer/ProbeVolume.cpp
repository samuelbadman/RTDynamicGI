#include "Pch.h"
#include "ProbeVolume.h"

float CalculateBias(const float spacing)
{
	return (spacing - static_cast<int32_t>(spacing)) > 0.0f ? 0.0f : 1.0f;
}

Renderer::ProbeVolume::ProbeVolume(const glm::vec3& position, const glm::vec3& volumeExtents, float probeSpacing, float debugProbeSize)
	: Position(position), Extents(volumeExtents), ProbeSpacing(probeSpacing)
{
	// Calculate the number of probes
	ProbeCountX = static_cast<size_t>(volumeExtents.x / probeSpacing);
	ProbeCountY = static_cast<size_t>(volumeExtents.y / probeSpacing);
	ProbeCountZ = static_cast<size_t>(volumeExtents.z / probeSpacing);
	auto probeCountTotal = ProbeCountX * ProbeCountY * ProbeCountZ;

	// Initialize probe transforms
	ProbeTransforms.resize(probeCountTotal, { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {debugProbeSize, debugProbeSize, debugProbeSize} });
	UpdateProbePositions();
}

void Renderer::ProbeVolume::Update()
{
	if (UpdatedPosition != Position)
	{
		UpdateProbePositions();
	}
}

void Renderer::ProbeVolume::UpdateProbePositions()
{
	for (auto x = 0; x < ProbeCountX; ++x)
	{
		for (auto y = 0; y < ProbeCountY; ++y)
		{
			for (auto z = 0; z < ProbeCountZ; ++z)
			{
				ProbeTransforms[x + ProbeCountX * (y + ProbeCountZ * z)].Position = Position + glm::vec3((x * ProbeSpacing) - ((Extents.x - ProbeSpacing) / 2.0f),
													   (y * ProbeSpacing) - ((Extents.y - ProbeSpacing) / 2.0f),
													   (z * ProbeSpacing) - ((Extents.z - ProbeSpacing) / 2.0f));
			}
		}
	}
}
