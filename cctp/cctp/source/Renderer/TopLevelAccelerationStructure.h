#pragma once

struct Transform;

namespace Renderer
{
	class BottomLevelAccelerationStructure;

	class TopLevelAccelerationStructure
	{
	public:
		TopLevelAccelerationStructure(ID3D12Device5* device, bool allowUpdate, uint32_t instanceCount);
		void SetInstanceBlasAndTransform(const uint32_t instanceID, const BottomLevelAccelerationStructure& blas, const glm::mat4& transformMatrix);
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& GetBuildDesc() const { return BuildDesc; }
		ID3D12Resource* GetTlasResource() const { return Tlas.Get(); }
		bool UpdateAllowed() const { return AllowUpdate; }
		uint32_t GetInstanceCount() const { return InstanceCount; }
		ID3D12Resource* GetInstancesBuffer() const { return InstancesBuffer.Get(); }
		ID3D12Resource* GetScratchBuffer() const { return Scratch.Get(); }

	private:
		bool AllowUpdate;
		uint32_t InstanceCount;
		Microsoft::WRL::ComPtr<ID3D12Resource> Tlas;
		Microsoft::WRL::ComPtr<ID3D12Resource> Scratch;
		Microsoft::WRL::ComPtr<ID3D12Resource> InstancesBuffer;
		D3D12_RAYTRACING_INSTANCE_DESC* MappedInstancesBufferLocation;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC BuildDesc;
	};
}