#pragma once

#include "Renderer/SwapChain.h"
#include "Renderer/Pipeline/GraphicsPipeline.h"
#include "Renderer/Pipeline/ScreenPassPipeline.h"
#include "Renderer/Pipeline/ShadowMapPassPipeline.h"

#include "Geometry.h"
#include "Mesh.h"
#include "Camera.h"
#include "BottomLevelAccelerationStructure.h"
#include "TopLevelAccelerationStructure.h"
#include "DescriptorHeap.h"

struct Transform;

namespace Renderer
{
	enum SHADER_VISIBLE_DESCRIPTOR_INDICES
	{
		IMGUI_DESCRIPTOR_INDEX = 0,

		SCENE_BVH_SRV_DESCRIPTOR_INDEX,
		RAYTRACE_IRRADIANCE_UAV_DESCRIPTOR_INDEX,
		RAYTRACE_VISIBILITY_UAV_DESCRIPTOR_INDEX,
		SCENE_SRV_DESCRIPTOR_INDEX,
		SCENE_DEPTH_SRV_DESCRIPTOR_INDEX,
		SHADOW_MAP_SRV_DESCRIPTOR_INDEX,
		RAYTRACE_IRRADIANCE_SRV_DESCRIPTOR_INDEX,
		RAYTRACE_VISIBILITY_SRV_DESCRIPTOR_INDEX,
		CUBE_VERTEX_BUFFER_SRV_DESCRIPTOR_INDEX,

		SHADER_VISIBLE_CBV_SRV_UAV_DESCRIPTOR_COUNT
	};

	constexpr glm::vec2 RAYTRACE_IRRADIANCE_OUTPUT_DIMS = glm::vec2(4300.0f, 16.0f);
	constexpr glm::vec2 RAYTRACE_VISIBILITY_OUTPUT_DIMS = glm::vec2(7000.0f, 32.0f);
	constexpr glm::vec2 SHADOW_MAP_DIMS = glm::vec2(1024.0f, 1024.0f);

	class Material;

	constexpr size_t MAX_MATERIAL_COUNT = 8;

	constexpr size_t MAX_PROBE_COUNT = 350;

	bool Init(const uint32_t shaderVisibleCBVSRVUAVDescriptorCount);
	bool Shutdown();
	bool Flush();
	bool CreateSwapChain(HWND windowHandle, UINT width, UINT height, DXGI_FORMAT format, std::unique_ptr<SwapChain>& swapChain);
	bool ResizeSwapChain(SwapChain* pSwapChain, UINT newWidth, UINT newHeight);
	template<typename T>
	bool CreateGraphicsPipeline(SwapChain* pSwapChain, std::unique_ptr<GraphicsPipelineBase>& pipeline);
	void CreateStagedMesh(const std::vector<Vertex1Pos1UV1Norm>& vertices, const std::vector<uint32_t>& indices,
		const std::wstring& name, std::unique_ptr<Mesh>& mesh);
	bool LoadStagedMeshesOntoGPU(std::unique_ptr<Mesh>* pMeshes, const size_t meshCount);
	void CreateBottomLevelAccelerationStructure(Mesh& mesh, std::unique_ptr<BottomLevelAccelerationStructure>& blas);
	bool BuildBottomLevelAccelerationStructures(std::unique_ptr<BottomLevelAccelerationStructure>* pStructures, const size_t structureCount);
	void CreateTopLevelAccelerationStructure(std::unique_ptr<TopLevelAccelerationStructure>& tlas, const bool allowUpdate, const uint32_t instanceCount);
	bool BuildTopLevelAccelerationStructures(std::unique_ptr<TopLevelAccelerationStructure>* pStructures, const size_t structureCount);
	// First descriptor index is occupied by ImGui resources
	void AddSRVDescriptorToShaderVisibleHeap(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, const uint32_t descriptorIndex);
	// First descriptor index is occupied by ImGui resources
	void AddUAVDescriptorToShaderVisibleHeap(ID3D12Resource* pResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, const uint32_t descriptorIndex);

	UINT GetRTDescriptorIncrementSize();
	UINT GetDSDescriptorIncrementSize();
	bool GetVSyncEnabled();
	void SetVSyncEnabled(const bool enabled);
	const DescriptorHeap* GetShaderVisibleDescriptorHeap();
	D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameConstantBufferGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS GetPerObjectConstantBufferGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS GetPerPassConstantBufferGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS GetMaterialConstantBufferGPUVirtualAddress();
	UINT64 GetConstantBufferAllignmentSize();

	// Temporary
	ID3D12Device5* GetDevice();

	// Commands
	namespace Commands
	{
		bool StartFrame(SwapChain* pSwapChain);
		bool EndFrame(SwapChain* pSwapChain);
		void ClearRenderTargets(SwapChain* pSwapChain, bool depthOnly, const CD3DX12_CPU_DESCRIPTOR_HANDLE& depthTargetDescriptorHandle);
		void SetBackBufferRenderTargets(SwapChain* pSwapChain, bool depthOnly, const CD3DX12_CPU_DESCRIPTOR_HANDLE& depthTargetDescriptorHandle);
		void SetPrimitiveTopology();
		void SetViewport(SwapChain* pSwapChain);
		void SetViewport(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect);
		void SetGraphicsPipeline(GraphicsPipelineBase* pPipeline);
		void UpdatePerFrameConstants(const std::vector<Transform>& probeTransformsWS, const glm::vec3& lightDirectionWS, const float lightIntensity, const float probeSpacing);
		void UpdatePerPassConstants(const uint32_t passIndex, const glm::vec2& viewportDims, const Camera& camera);
		void UpdateMaterialConstants(const Renderer::Material* pMaterials, const uint32_t materialCount);
		void SubmitMesh(UINT perObjectConstantsParameterIndex, const Mesh& mesh, const Transform& transform, const glm::vec4& color, const bool lit);
		void SubmitScreenMesh(const Mesh& mesh);
		void SetDescriptorHeaps();
		void BeginImGui();
		void EndImGui();
		void RebuildTlas(TopLevelAccelerationStructure* tlas);
		void Raytrace(const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc, ID3D12StateObject* pPipelineStateObject, ID3D12Resource* pRaytraceOutputResource, ID3D12Resource* pRaytraceOutput2Resource);
		void SetGraphicsDescriptorTableRootParam(UINT rootParameterIndex, const uint32_t baseDescriptorIndex);
		void SetGraphicsConstantBufferViewRootParam(UINT rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS bufferAddress);

		// Copies the src resource to the current frame's swap chain backbuffer. Swap chain render target resource is returned to render target
		// state after copy. Src resource is returned to srcResourceState after copy
		void DebugCopyResourceToRenderTarget(SwapChain* pSwapChain, ID3D12Resource* pSrcResource, D3D12_RESOURCE_STATES srcResourceState);
		void CopyRenderTargetToResource(SwapChain* pSwapChain, ID3D12Resource* pDstResource, D3D12_RESOURCE_STATES dstResourceState);
		void CopyDepthTargetToResource(ID3D12Resource* pDepthTarget, ID3D12Resource* pDstResource, D3D12_RESOURCE_STATES dstResourceState);
	}
}