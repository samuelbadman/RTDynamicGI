#pragma once

namespace Renderer
{
	class GraphicsPipelineBase
	{
	public:
		virtual ~GraphicsPipelineBase() = default;

		ID3D12PipelineState* GetPipelineState() const { return PipelineStateObject.Get(); }
		ID3D12RootSignature* GetRootSignature() const { return RootSignature.Get(); }

		virtual bool Init(ID3D12Device* pDevice, DXGI_FORMAT renderTargetFormat) = 0;

	protected:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStateObject;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
	};
}