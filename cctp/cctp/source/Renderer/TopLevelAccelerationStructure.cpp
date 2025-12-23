#include "Pch.h"
#include "TopLevelAccelerationStructure.h"
#include "BottomLevelAccelerationStructure.h"

Renderer::TopLevelAccelerationStructure::TopLevelAccelerationStructure(ID3D12Device5* device, bool allowUpdate, uint32_t instanceCount)
	: AllowUpdate(allowUpdate), InstanceCount(instanceCount)
{
	// Create GPU resource for instance descriptions
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount);

	if (FAILED(device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&InstancesBuffer))))
	{
		assert(false && "Failed to create GPU resource for tlas instance descriptions buffer.");
	}

	// Map the instance description buffer GPU resource
	if (FAILED(InstancesBuffer->Map(0, nullptr, (void**)&MappedInstancesBufferLocation)))
	{
		assert(false && "Failed to map instance buffer during tlas construction.");
	}

	// Query memory requirements for GPU resources
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = AllowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
		: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = InstanceCount;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create GPU resources for tlas and scratch buffers
	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	if (FAILED(device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&scratchDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&Scratch)
	)))
	{
		assert(false && "Failed to create GPU resource for tlas scratch buffer.");
	}

	auto resultDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	if (FAILED(device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resultDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(&Tlas))))
	{
		assert(false && "Failed to create GPU resource for tlas.");
	}

	BuildDesc.Inputs = inputs;
	BuildDesc.Inputs.InstanceDescs = InstancesBuffer->GetGPUVirtualAddress();
	BuildDesc.DestAccelerationStructureData = Tlas->GetGPUVirtualAddress();
	BuildDesc.ScratchAccelerationStructureData = Scratch->GetGPUVirtualAddress();
}

void Renderer::TopLevelAccelerationStructure::SetInstanceBlasAndTransform(const uint32_t instanceID, const BottomLevelAccelerationStructure& blas, const glm::mat4& transformMatrix)
{
	assert(instanceID < InstanceCount && "Setting instance transform with invalid instance ID.");

	MappedInstancesBufferLocation[instanceID].InstanceID = instanceID;
	MappedInstancesBufferLocation[instanceID].InstanceContributionToHitGroupIndex = 0;
	MappedInstancesBufferLocation[instanceID].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	auto transformMatrixTransposed = glm::transpose(transformMatrix);
	memcpy(MappedInstancesBufferLocation[instanceID].Transform, &transformMatrixTransposed, sizeof(MappedInstancesBufferLocation[instanceID].Transform));
	MappedInstancesBufferLocation[instanceID].AccelerationStructure = blas.GetBlas()->GetGPUVirtualAddress();
	MappedInstancesBufferLocation[instanceID].InstanceMask = 0xFF;
}
