#pragma once

class DescriptorHeap
{
public:
	void Init(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors,
		const bool shaderVisible);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const uint32_t descriptorOffset) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const uint32_t descriptorOffset) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const;
	ID3D12DescriptorHeap* Get() const { return Heap.Get(); }
	uint32_t GetNumDescriptors() const { return NumDescriptors; }

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	size_t CPUHandle = {};
	size_t GPUHandle = {};
	uint32_t DescriptorIncrementSize = 0;
	uint32_t NumDescriptors = 0;
};