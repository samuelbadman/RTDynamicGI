#pragma once

enum class SamplerType : uint8_t;

class RootSignature
{
public:
	~RootSignature();

	void AddRootDescriptorTableParameter(D3D12_DESCRIPTOR_RANGE* const descriptorRanges, const uint32_t numRanges,
		D3D12_SHADER_VISIBILITY shaderVisibility);
	void AddRootDescriptorTableParameter(std::initializer_list<D3D12_DESCRIPTOR_RANGE> descriptorRanges,
		D3D12_SHADER_VISIBILITY shaderVisibility);
	void AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE type, const uint32_t shaderRegister,
		const uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility);
	void AddStaticSampler(const SamplerType type,
		const uint32_t shaderRegister, const uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility);
	void SetFlags(const D3D12_ROOT_SIGNATURE_FLAGS flags);
	void Create(ID3D12Device* const device);
	ID3D12RootSignature* GetRootSignature() const { return RootSignature.Get(); }

private:
	std::vector<D3D12_DESCRIPTOR_RANGE*> DescriptorRanges = {};
	std::vector<D3D12_ROOT_PARAMETER> Parameters = {};
	std::vector<D3D12_STATIC_SAMPLER_DESC> StaticSamplers = {};
	D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
};
