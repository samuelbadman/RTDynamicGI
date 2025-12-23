#include "Pch.h"
#include "DescriptorHeap.h"

void DescriptorHeap::Init(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors, const bool shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = type;
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&Heap));
	assert(SUCCEEDED(hr));
	CPUHandle = Heap->GetCPUDescriptorHandleForHeapStart().ptr;
	GPUHandle = Heap->GetGPUDescriptorHandleForHeapStart().ptr;
	DescriptorIncrementSize = device->GetDescriptorHandleIncrementSize(type);
	NumDescriptors = numDescriptors;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUDescriptorHandleForHeapStart() const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = GPUHandle;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUDescriptorHandle(const uint32_t descriptorOffset) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = CPUHandle + (static_cast<size_t>(DescriptorIncrementSize) * descriptorOffset);
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUDescriptorHandleForHeapStart() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = CPUHandle;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUDescriptorHandle(const uint32_t descriptorOffset) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = GPUHandle + (static_cast<size_t>(DescriptorIncrementSize) * descriptorOffset);
	return handle;
}
