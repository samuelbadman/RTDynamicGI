#pragma once

#include "GraphicsPipelineBase.h"

namespace Renderer
{
	class ScreenPassPipeline : public GraphicsPipelineBase
	{
	public:
		bool Init(ID3D12Device* pDevice, DXGI_FORMAT renderTargetFormat) final;
	};
}