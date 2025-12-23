#pragma once

#include "GraphicsPipelineBase.h"

namespace Renderer
{
	class ShadowMapPassPipeline : public GraphicsPipelineBase
	{
	public:
		bool Init(ID3D12Device* pDevice, DXGI_FORMAT renderTargetFormat) final;

	private:

	};
}