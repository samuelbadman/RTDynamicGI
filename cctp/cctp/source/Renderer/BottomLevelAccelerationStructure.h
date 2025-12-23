#pragma once

namespace Renderer
{
	class Mesh;

	class BottomLevelAccelerationStructure
	{
	public:
		BottomLevelAccelerationStructure(ID3D12Device5* device, Mesh& mesh);
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& GetBuildDesc() const { return BuildDesc; }
		ID3D12Resource* GetBlas() const { return Blas.Get(); }

	private:
		uint32_t GeometryID = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> Blas;
		Microsoft::WRL::ComPtr<ID3D12Resource> Scratch;
		D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC BuildDesc;
	};
}