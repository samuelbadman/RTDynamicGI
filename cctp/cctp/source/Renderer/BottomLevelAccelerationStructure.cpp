#include "Pch.h"
#include "BottomLevelAccelerationStructure.h"
#include "Mesh.h"

Renderer::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(ID3D12Device5* device, Mesh& mesh)
{
	// Describe the geometry
	GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	GeometryDesc.Triangles.VertexBuffer.StartAddress = mesh.GetVertexBuffer()->GetGPUVirtualAddress();
	GeometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex1Pos1UV1Norm);
	GeometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	GeometryDesc.Triangles.VertexCount = mesh.GetVertexCount();
	GeometryDesc.Triangles.IndexBuffer = mesh.GetIndexBuffer()->GetGPUVirtualAddress();
	GeometryDesc.Triangles.IndexCount = mesh.GetIndexCount();
	GeometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	GeometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // Use D3D12_RAYTRACING_GEOMETRY_FLAG_NONE if geometry is not opaque

	// Query blas memory requirements
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &GeometryDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create GPU resources for the blas buffer
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	if (FAILED(device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(&Blas))))
	{
		assert(false && "Failed to create resource for blas.");
	}

	// Create GPU resources for the scratch buffer
	if (FAILED(device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&Scratch))))
	{
		assert(false && "Failed to create scratch resource for blas.");
	}

	// Fill out build description
	BuildDesc.Inputs = inputs;
	BuildDesc.DestAccelerationStructureData = Blas->GetGPUVirtualAddress();
	BuildDesc.ScratchAccelerationStructureData = Scratch->GetGPUVirtualAddress();
}
