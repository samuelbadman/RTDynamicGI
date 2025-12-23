#pragma once

#include "Renderer/Vertices/Vertex1Pos1UV1Norm.h"

namespace Renderer
{
	namespace Geometry
	{
		void GenerateCubeGeometry(std::vector<Vertex1Pos1UV1Norm>& outVertices, std::vector<uint32_t>& outIndices, const float width);
		void GenerateSphereGeometry(std::vector<Vertex1Pos1UV1Norm>& outVertices, std::vector<uint32_t>& outIndices, 
			const float radius, const int32_t sectors, const int32_t stacks);
	}
}