#include "Pch.h"
#include "Window/Window.h"
#include "Events/EventSystem.h"
#include "Renderer/Renderer.h"
#include "Binary/Binary.h"
#include "Math/Math.h"

#include "Scene/Scenes/DemoScene.h"

#include "Renderer/RootSignature.h"
#include "Renderer/SamplerType.h"

#define ALIGN_TO(size, alignment) (size + (alignment - 1) & ~(alignment-1))

constexpr glm::vec2 WINDOW_DIMS = glm::vec2(1920.0f, 1080.0f);

void CreateConsole(const uint32_t maxLines)
{
	CONSOLE_SCREEN_BUFFER_INFO console_info;
	FILE* fp;
	// allocate a console for this app
	AllocConsole();
	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_info);
	console_info.dwSize.Y = maxLines;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), console_info.dwSize);
	// redirect unbuffered STDOUT to the console
	if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
		if (!(freopen_s(&fp, "CONOUT$", "w", stdout) != 0))
			setvbuf(stdout, NULL, _IONBF, 0);
	// redirect unbuffered STDIN to the console
	if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
		if (!(freopen_s(&fp, "CONIN$", "r", stdin) != 0))
			setvbuf(stdin, NULL, _IONBF, 0);
	// redirect unbuffered STDERR to the console
	if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
		if (!(freopen_s(&fp, "CONOUT$", "w", stderr) != 0))
			setvbuf(stderr, NULL, _IONBF, 0);
	// make C++ standard streams point to console as well
	std::ios::sync_with_stdio(true);
	// clear the error state for each of the C++ standard streams
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
}

void ReleaseConsole()
{
	FILE* fp;
	// Just to be safe, redirect standard IO to NULL before releasing.
	// Redirect STDIN to NULL
	if (!(freopen_s(&fp, "NUL:", "r", stdin) != 0))
		setvbuf(stdin, NULL, _IONBF, 0);
	// Redirect STDOUT to NULL
	if (!(freopen_s(&fp, "NUL:", "w", stdout) != 0))
		setvbuf(stdout, NULL, _IONBF, 0);
	// Redirect STDERR to NULL
	if (!(freopen_s(&fp, "NUL:", "w", stderr) != 0))
		setvbuf(stderr, NULL, _IONBF, 0);
	// Detach from console
	FreeConsole();
}

int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#ifdef _DEBUG
	CreateConsole(2048);
#endif

	// Init window
	if (!Window::Init(L"Demo window", WINDOW_DIMS, Window::STYLE_NO_RESIZE))
	{
		assert(false && "Failed to initialize window.");
	}

	Window::Show(SW_MAXIMIZE);

	// Subscribe input event handler
	EventSystem::SubscribeToEvent<InputEvent>([](InputEvent&& event)
		{
			// If escape key pressed
			if (event.Input == InputCodes::Escape &&
				event.Data == 1.0f &&
				!event.RepeatedKey)
			{
				// Close the window
				Window::Close();
			}
		});

	// Init renderer
	if (!Renderer::Init(Renderer::SHADER_VISIBLE_CBV_SRV_UAV_DESCRIPTOR_COUNT))
	{
		assert(false && "Failed to initialize renderer.");
	}

	Renderer::SetVSyncEnabled(true);

	// Create a swap chain for the window
	std::unique_ptr<Renderer::SwapChain> swapChain;
	RECT clientRect;
	if (!Window::GetClientAreaRect(clientRect))
	{
		assert(false && "Failed to get the window client area rect.");
	}

	const auto clientRectWidth = clientRect.right - clientRect.left;
	const auto clientRectHeight = clientRect.bottom - clientRect.top;

	if (!Renderer::CreateSwapChain(Window::GetHandle(), clientRectWidth, clientRectHeight, DXGI_FORMAT_R8G8B8A8_UNORM, swapChain))
	{
		assert(false && "Failed to create a swap chain for the window.");
	}

	// Subscribe window resized event handler
	//EventSystem::SubscribeToEvent<WindowResizedEvent>([&swapChain](WindowResizedEvent&& event)
	//	{
	//		// Get the resized client area width and height
	//		RECT clientRect;
	//		Window::GetClientAreaRect(clientRect);
	//		auto newWidth = clientRect.right - clientRect.left;
	//		auto newHeight = clientRect.bottom - clientRect.top;

	//		// Check the new width and height are greater than 0
	//		if ((newWidth <= 0) && (newHeight <= 0))
	//		{
	//			return;
	//		}

	//		// Wait for GPU queues to idle
	//		Renderer::Flush();

	//		// Resize the swap chain
	//		Renderer::ResizeSwapChain(swapChain.get(), static_cast<UINT>(512), static_cast<UINT>(512));
	//	});

	// Create graphics pipeline
	std::unique_ptr<Renderer::GraphicsPipelineBase> graphicsPipeline;
	if (!Renderer::CreateGraphicsPipeline<Renderer::GraphicsPipeline>(swapChain.get(), graphicsPipeline))
	{
		assert(false && "Failed to create graphics pipeline.");
	}

	// Create screen pass graphics pipeline
	std::vector<Renderer::Vertex1Pos1UV1Norm> screenVertices(4);
	screenVertices[0].Position = glm::vec3(-1.0f, 1.0f, 0.f);
	screenVertices[0].UV = glm::vec2(0.0f, 1.0f);
	screenVertices[0].Normal = glm::vec3(0.0f, 0.0f, -1.0f);
	screenVertices[1].Position = glm::vec3(1.0f, 1.0f, 0.f);
	screenVertices[1].UV = glm::vec2(1.0f, 1.0f);
	screenVertices[1].Normal = glm::vec3(0.0f, 0.0f, -1.0f);
	screenVertices[2].Position = glm::vec3(-1.0f, -1.0f, 0.f);
	screenVertices[2].UV = glm::vec2(0.0f, 0.0f);
	screenVertices[2].Normal = glm::vec3(0.0f, 0.0f, -1.0f);
	screenVertices[3].Position = glm::vec3(1.0f, -1.0f, 0.f);
	screenVertices[3].UV = glm::vec2(1.0f, 0.0f);
	screenVertices[3].Normal = glm::vec3(0.0f, 0.0f, -1.0f);

	std::vector<uint32_t> screenIndices({ 0, 1, 2, 2, 1, 3 });

	std::unique_ptr<Renderer::Mesh> screenMesh;
	Renderer::CreateStagedMesh(screenVertices, screenIndices, L"ScreenMesh", screenMesh);

	if (!Renderer::LoadStagedMeshesOntoGPU(&screenMesh, 1))
	{
		assert(false && "Failed to load screen mesh to GPU.");
	}

	std::unique_ptr<Renderer::GraphicsPipelineBase> screenPassPipeline;
	if (!Renderer::CreateGraphicsPipeline<Renderer::ScreenPassPipeline>(swapChain.get(), screenPassPipeline))
	{
		assert(false && "Failed to create screen pass pipeline.");
	}

	// Create shadow map pass pipeline
	std::unique_ptr<Renderer::GraphicsPipelineBase> shadowMapPassPipeline;
	if (!Renderer::CreateGraphicsPipeline<Renderer::ShadowMapPassPipeline>(swapChain.get(), shadowMapPassPipeline))
	{
		assert(false && "Failed to create shadow map pass pipeline.");
	}

	// Create demo scene
	auto demoScene = std::make_unique<DemoScene>();

	// Create raytracing pipeline

	// Create raytracing resources and add descriptors to resources
	// Scene bvh 
	D3D12_SHADER_RESOURCE_VIEW_DESC sceneBVHSRVDesc = {};
	sceneBVHSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	sceneBVHSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	sceneBVHSRVDesc.RaytracingAccelerationStructure.Location = demoScene->GetTlas()->GetTlasResource()->GetGPUVirtualAddress();
	Renderer::AddSRVDescriptorToShaderVisibleHeap(nullptr, &sceneBVHSRVDesc, Renderer::SCENE_BVH_SRV_DESCRIPTOR_INDEX);

	// Create GBuffer
	// Raytracing output texture (irradiance)
	Microsoft::WRL::ComPtr<ID3D12Resource> raytraceOutputResource;

	auto raytraceOutputTextureResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R11G11B10_FLOAT,
		static_cast<UINT64>(Renderer::RAYTRACE_IRRADIANCE_OUTPUT_DIMS.x), static_cast<UINT64>(Renderer::RAYTRACE_IRRADIANCE_OUTPUT_DIMS.y));
	raytraceOutputTextureResourceDesc.MipLevels = 1;
	raytraceOutputTextureResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	auto raytraceOutputTextureHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	if (FAILED(Renderer::GetDevice()->CreateCommittedResource(&raytraceOutputTextureHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&raytraceOutputTextureResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&raytraceOutputResource))))
	{
		assert(false && "Failed to create raytrace output texture resource.");
	}
	Renderer::AddUAVDescriptorToShaderVisibleHeap(raytraceOutputResource.Get(), nullptr, Renderer::RAYTRACE_IRRADIANCE_UAV_DESCRIPTOR_INDEX);
	Renderer::AddSRVDescriptorToShaderVisibleHeap(raytraceOutputResource.Get(), nullptr, Renderer::RAYTRACE_IRRADIANCE_SRV_DESCRIPTOR_INDEX);

	// Raytracing output 2 texture (visibility)
	Microsoft::WRL::ComPtr<ID3D12Resource> raytraceOutput2Resource;

	auto raytraceOutput2TextureResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16_FLOAT,
		static_cast<UINT64>(Renderer::RAYTRACE_VISIBILITY_OUTPUT_DIMS.x), static_cast<UINT64>(Renderer::RAYTRACE_VISIBILITY_OUTPUT_DIMS.y));
	raytraceOutput2TextureResourceDesc.MipLevels = 1;
	raytraceOutput2TextureResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	auto raytraceOutput2TextureHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	if (FAILED(Renderer::GetDevice()->CreateCommittedResource(&raytraceOutput2TextureHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&raytraceOutput2TextureResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&raytraceOutput2Resource))))
	{
		assert(false && "Failed to create raytrace output 2 texture resource.");
	}
	Renderer::AddUAVDescriptorToShaderVisibleHeap(raytraceOutput2Resource.Get(), nullptr, Renderer::RAYTRACE_VISIBILITY_UAV_DESCRIPTOR_INDEX);
	Renderer::AddSRVDescriptorToShaderVisibleHeap(raytraceOutput2Resource.Get(), nullptr, Renderer::RAYTRACE_VISIBILITY_SRV_DESCRIPTOR_INDEX);

	// Scene texture
	Microsoft::WRL::ComPtr<ID3D12Resource> sceneBufferResource;

	auto sceneBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(swapChain->GetFormat(),
		static_cast<UINT>(swapChain->GetViewportWidth()), static_cast<UINT>(swapChain->GetViewportHeight()));
	sceneBufferDesc.MipLevels = 1;
	auto sceneBufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	if (FAILED(Renderer::GetDevice()->CreateCommittedResource(&sceneBufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&sceneBufferDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&sceneBufferResource))))
	{
		assert(false && "Failed to create scene buffer resource.");
	}
	Renderer::AddSRVDescriptorToShaderVisibleHeap(sceneBufferResource.Get(), nullptr, Renderer::SCENE_SRV_DESCRIPTOR_INDEX);

	// Scene depth texture
	Microsoft::WRL::ComPtr<ID3D12Resource> sceneDepthBufferResource;

	auto sceneDepthBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT,
		static_cast<UINT>(swapChain->GetViewportWidth()), static_cast<UINT>(swapChain->GetViewportHeight()));
	sceneDepthBufferDesc.MipLevels = 11;
	auto sceneDepthBufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	if (FAILED(Renderer::GetDevice()->CreateCommittedResource(&sceneDepthBufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&sceneDepthBufferDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&sceneDepthBufferResource))))
	{
		assert(false && "Failed to create scene depth buffer resource.");
	}
	Renderer::AddSRVDescriptorToShaderVisibleHeap(sceneDepthBufferResource.Get(), nullptr, Renderer::SCENE_DEPTH_SRV_DESCRIPTOR_INDEX);

	// Shadow map texture
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowMapBufferResource;

	auto shadowMapBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT,
		static_cast<UINT>(Renderer::SHADOW_MAP_DIMS.x), static_cast<UINT>(Renderer::SHADOW_MAP_DIMS.y));
	auto shadowMapBufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	if (FAILED(Renderer::GetDevice()->CreateCommittedResource(&shadowMapBufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&shadowMapBufferDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&shadowMapBufferResource))))
	{
		assert(false && "Failed to create shadow map buffer resource.");
	}
	Renderer::AddSRVDescriptorToShaderVisibleHeap(shadowMapBufferResource.Get(), nullptr, Renderer::SHADOW_MAP_SRV_DESCRIPTOR_INDEX);

	// Create shadow map depth stencil target
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowMapDepthStencilBuffer;

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	auto shadowMapDSVHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto shadowMapDSVResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, static_cast<UINT64>(Renderer::SHADOW_MAP_DIMS.x), static_cast<UINT64>(Renderer::SHADOW_MAP_DIMS.y),
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	if (FAILED(Renderer::GetDevice()->CreateCommittedResource(&shadowMapDSVHeapProperties, D3D12_HEAP_FLAG_NONE,
		&shadowMapDSVResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&shadowMapDepthStencilBuffer)
	)))
	{
		assert(false && "Failed to create shadow map depth stencil resource.");
	}

	Renderer::GetDevice()->CreateDepthStencilView(shadowMapDepthStencilBuffer.Get(), &depthStencilDesc, swapChain->GetShadowMapDSDescriptorHandle()); // Instead of using a descriptor heap per 
																																					  // swap chain. Create a global dsv heap 
																																					  // that is used by all objects requiring a dsv

	// Load compiled raytracing shaders
	BinaryBuffer rayGenBuffer;
	if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/RayGen.dxil", rayGenBuffer))
	{
		assert(false && "Failed to read compiled ray gen shader.");
	}

	BinaryBuffer closestHitBuffer;
	if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/ClosestHit.dxil", closestHitBuffer))
	{
		assert(false && "Failed to read compiled closest hit shader.");
	}

	BinaryBuffer missBuffer;
	if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/Miss.dxil", missBuffer))
	{
		assert(false && "Failed to read compiled miss shader.");
	}

	// Create raytracing pipeline state object
	Microsoft::WRL::ComPtr<ID3D12StateObject> raytracingPipelineStateObject;

	CD3DX12_STATE_OBJECT_DESC rtpsoDesc = {};
	rtpsoDesc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	// Add ray gen shader
	constexpr LPCWSTR rayGenExportName = L"RayGen";
	CD3DX12_DXIL_LIBRARY_SUBOBJECT rayGenLibSubobject = {};
	auto rayGenBytecode = CD3DX12_SHADER_BYTECODE(rayGenBuffer.GetBufferPointer(), rayGenBuffer.GetBufferLength());
	rayGenLibSubobject.SetDXILLibrary(&rayGenBytecode);
	rayGenLibSubobject.DefineExport(rayGenExportName);
	rayGenLibSubobject.AddToStateObject(rtpsoDesc);

	// Add miss shader
	constexpr LPCWSTR missExportName = L"Miss";
	CD3DX12_DXIL_LIBRARY_SUBOBJECT missLibSubobject = {};
	auto missBytecode = CD3DX12_SHADER_BYTECODE(missBuffer.GetBufferPointer(), missBuffer.GetBufferLength());
	missLibSubobject.SetDXILLibrary(&missBytecode);
	missLibSubobject.DefineExport(missExportName);
	missLibSubobject.AddToStateObject(rtpsoDesc);

	// Add closest hit shader
	constexpr LPCWSTR closestHitExportName = L"ClosestHit";
	CD3DX12_DXIL_LIBRARY_SUBOBJECT closestHitLibSubobject = {};
	auto closestHitBytecode = CD3DX12_SHADER_BYTECODE(closestHitBuffer.GetBufferPointer(), closestHitBuffer.GetBufferLength());
	closestHitLibSubobject.SetDXILLibrary(&closestHitBytecode);
	closestHitLibSubobject.DefineExport(closestHitExportName);
	closestHitLibSubobject.AddToStateObject(rtpsoDesc);

	// Add hit group shader
	constexpr LPCWSTR hitGroupExportName = L"HitGroup";
	CD3DX12_HIT_GROUP_SUBOBJECT hitGroupSubobject = {};
	hitGroupSubobject.SetIntersectionShaderImport(nullptr);
	hitGroupSubobject.SetAnyHitShaderImport(nullptr);
	hitGroupSubobject.SetClosestHitShaderImport(closestHitExportName);
	hitGroupSubobject.SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	hitGroupSubobject.SetHitGroupExport(hitGroupExportName);
	hitGroupSubobject.AddToStateObject(rtpsoDesc);

	// Add shader config subobject
	UINT payloadSize = sizeof(float) * 4;
	UINT attributeSize = sizeof(float) * 2;
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT shaderConfigSubobject = {};
	shaderConfigSubobject.Config(payloadSize, attributeSize);
	shaderConfigSubobject.AddToStateObject(rtpsoDesc);

	// Create ray gen shader local root signature
	RootSignature rayGenRootSignature;

	D3D12_DESCRIPTOR_RANGE rayGenDescriptorRanges[3];

	rayGenDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rayGenDescriptorRanges[0].NumDescriptors = 1;
	rayGenDescriptorRanges[0].BaseShaderRegister = 0;
	rayGenDescriptorRanges[0].RegisterSpace = 0;
	rayGenDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rayGenDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	rayGenDescriptorRanges[1].NumDescriptors = 1;
	rayGenDescriptorRanges[1].BaseShaderRegister = 0;
	rayGenDescriptorRanges[1].RegisterSpace = 0;
	rayGenDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rayGenDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	rayGenDescriptorRanges[2].NumDescriptors = 1;
	rayGenDescriptorRanges[2].BaseShaderRegister = 1;
	rayGenDescriptorRanges[2].RegisterSpace = 0;
	rayGenDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rayGenRootSignature.AddRootDescriptorTableParameter(rayGenDescriptorRanges, _countof(rayGenDescriptorRanges), D3D12_SHADER_VISIBILITY_ALL);
	rayGenRootSignature.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rayGenRootSignature.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	rayGenRootSignature.Create(Renderer::GetDevice());

	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT rayGenRootSignatureSubObject = {};
	rayGenRootSignatureSubObject.SetRootSignature(rayGenRootSignature.GetRootSignature());
	rayGenRootSignatureSubObject.AddToStateObject(rtpsoDesc);

	// Create association sub object for ray gen shader and ray gen root signature
	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT rayGenAssociationSubObject = {};
	rayGenAssociationSubObject.AddExport(rayGenExportName);
	rayGenAssociationSubObject.SetSubobjectToAssociate(rayGenRootSignatureSubObject);
	rayGenAssociationSubObject.AddToStateObject(rtpsoDesc);

	// Create hit group local root signature
	RootSignature hitGroupRootSignature;

	hitGroupRootSignature.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	hitGroupRootSignature.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	hitGroupRootSignature.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 2, 0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE closestHitDescriptorRanges[2];

	// Shadow map srv
	closestHitDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	closestHitDescriptorRanges[0].NumDescriptors = 1;
	closestHitDescriptorRanges[0].BaseShaderRegister = 0;
	closestHitDescriptorRanges[0].RegisterSpace = 0;
	closestHitDescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;

	// Cube vertex buffer srv
	closestHitDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	closestHitDescriptorRanges[1].NumDescriptors = 1;
	closestHitDescriptorRanges[1].BaseShaderRegister = 1;
	closestHitDescriptorRanges[1].RegisterSpace = 0;
	closestHitDescriptorRanges[1].OffsetInDescriptorsFromTableStart = 2;

	hitGroupRootSignature.AddRootDescriptorTableParameter(closestHitDescriptorRanges, _countof(closestHitDescriptorRanges), D3D12_SHADER_VISIBILITY_ALL);

	hitGroupRootSignature.AddStaticSampler(SamplerType::PointBorder, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

	hitGroupRootSignature.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	hitGroupRootSignature.Create(Renderer::GetDevice());

	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT hitGroupRootSignatureSubObject = {};
	hitGroupRootSignatureSubObject.SetRootSignature(hitGroupRootSignature.GetRootSignature());
	hitGroupRootSignatureSubObject.AddToStateObject(rtpsoDesc);

	// Create association sub object for hit group shader and hit group root signature
	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT hitGroupAssociationSubObject = {};
	hitGroupAssociationSubObject.AddExport(hitGroupExportName);
	hitGroupAssociationSubObject.SetSubobjectToAssociate(hitGroupRootSignatureSubObject);
	hitGroupAssociationSubObject.AddToStateObject(rtpsoDesc);

	// Create pipeline global root signature
	RootSignature raytracingGlobalRootSignature;
	raytracingGlobalRootSignature.Create(Renderer::GetDevice());

	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT raytracingGlobalRootSignatureSubObject = {};
	raytracingGlobalRootSignatureSubObject.SetRootSignature(raytracingGlobalRootSignature.GetRootSignature());
	raytracingGlobalRootSignatureSubObject.AddToStateObject(rtpsoDesc);

	// Create raytracing pipeline config subobject
	UINT raytraceRecursionDepth = 1;

	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT raytracingPipelineConfigSubObject = {};
	raytracingPipelineConfigSubObject.Config(raytraceRecursionDepth);
	raytracingPipelineConfigSubObject.AddToStateObject(rtpsoDesc);

	// Create raytracing pipeline state object
	if (FAILED(Renderer::GetDevice()->CreateStateObject(rtpsoDesc, IID_PPV_ARGS(&raytracingPipelineStateObject))))
	{
		assert(false && "Failed to create raytracing pipeline state object.");
	}

	// Create raytracing shader table

	// Calculate shader table size
	constexpr uint32_t shaderRecordCount = 1;
	// Shader identifier size + another 32 byte block for root arguments to meet alignment requirements
	constexpr uint32_t rayGenShaderRecordSize = ALIGN_TO(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 1, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	constexpr uint32_t missShaderRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	constexpr uint32_t hitGroupShaderRecordSize = ALIGN_TO(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 1, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	constexpr uint32_t shaderTableSize = rayGenShaderRecordSize + ALIGN_TO(missShaderRecordSize, 64) + ALIGN_TO(hitGroupShaderRecordSize, 64);

	// Create shader table GPU memory
	Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable;
	auto shaderTableHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto shaderTableResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(shaderTableSize);
	Renderer::GetDevice()->CreateCommittedResource(&shaderTableHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&shaderTableResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&shaderTable)
	);

	// Map shader table memory
	uint8_t* pShaderTableStart;
	if (FAILED(shaderTable->Map(0, nullptr, reinterpret_cast<void**>(&pShaderTableStart))))
	{
		assert(false && "Failed to map shader table GPU memory.");
	}

	// Get raytracing pipeline state object properties to query shader identifiers
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> raytracingPipelineStateObjectProperties;
	if (FAILED(raytracingPipelineStateObject->QueryInterface(IID_PPV_ARGS(&raytracingPipelineStateObjectProperties))))
	{
		assert(false && "Failed to query raytracing pipeline state object properties.");
	}

	// Populate shader table

	// Shader record 0: Ray gen
	// Shader identifier + descriptor table + root descriptor
	memcpy(pShaderTableStart, 
		raytracingPipelineStateObjectProperties->GetShaderIdentifier(rayGenExportName),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	*(uint64_t*)(pShaderTableStart + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = 
		(Renderer::GetShaderVisibleDescriptorHeap()->GetGPUDescriptorHandle(Renderer::RAYTRACE_IRRADIANCE_UAV_DESCRIPTOR_INDEX).ptr - 8); // This is a Pointer to the start of a descriptor range (UAV x 2) 
																																		  // in a descriptor table
																																		  
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pShaderTableStart + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) = Renderer::GetPerFrameConstantBufferGPUVirtualAddress();

	// Shader record 1: Miss
	// Shader identifier
	memcpy(pShaderTableStart + rayGenShaderRecordSize,
		raytracingPipelineStateObjectProperties->GetShaderIdentifier(missExportName),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Shader record 2: Hit group
	// Shader identifier + root descriptor + root descriptor + root descriptor + descriptor table
	memcpy(pShaderTableStart + rayGenShaderRecordSize + (missShaderRecordSize + 32), // Adding 32 bytes of padding to miss shader record for 64 byte table allignment requirement
		raytracingPipelineStateObjectProperties->GetShaderIdentifier(hitGroupExportName),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pShaderTableStart + rayGenShaderRecordSize + (missShaderRecordSize + 32) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) =
		Renderer::GetMaterialConstantBufferGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pShaderTableStart + rayGenShaderRecordSize + (missShaderRecordSize + 32) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) =
		Renderer::GetPerFrameConstantBufferGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pShaderTableStart + rayGenShaderRecordSize + (missShaderRecordSize + 32) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 + 8) =
		Renderer::GetPerPassConstantBufferGPUVirtualAddress();
	*(uint64_t*)(pShaderTableStart + rayGenShaderRecordSize + (missShaderRecordSize + 32) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 + 8 + 8) =
		(Renderer::GetShaderVisibleDescriptorHeap()->GetGPUDescriptorHandle(Renderer::SHADOW_MAP_SRV_DESCRIPTOR_INDEX).ptr);

	// Begin demo scene
	demoScene->Begin();

	// Enter main loop
	bool quit = false;
	auto lastFrameTime = std::chrono::high_resolution_clock::now();
	auto lastGIGatherTime = std::chrono::high_resolution_clock::now();
	while (!quit)
	{
		// Calculate frame delta time
		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float, std::milli> frameTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;
		auto frameTimeF = frameTime.count();

		// Handle OS messages
		if (Window::RunOSMessageLoop())
		{
			quit = true;
			continue;
		}

		// Tick demo scene
		demoScene->Tick(frameTimeF);

		// Start a frame for the swap chain, retrieving the current back buffer index to render to
		auto* pSwapChain = swapChain.get();
		Renderer::Commands::StartFrame(pSwapChain);

		// Set primitive topology
		Renderer::Commands::SetPrimitiveTopology();

		// Set descriptor heaps
		Renderer::Commands::SetDescriptorHeaps();

		// Update per frame constants
		static auto& probeVolume = demoScene->GetProbeVolume();
		static const auto& lightDirection = demoScene->GetLightDirectionWS();
		Renderer::Commands::UpdatePerFrameConstants(probeVolume.GetProbeTransforms(), lightDirection, demoScene->GetLightIntensity(), demoScene->GetProbeVolume().GetProbeSpacing());

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//// Render shadow map pass
		uint32_t passIndex = 0;

		// Set pipeline
		Renderer::Commands::SetGraphicsPipeline(shadowMapPassPipeline.get());

		// Set per frame constant buffer view for pipeline
		Renderer::Commands::SetGraphicsConstantBufferViewRootParam(1, Renderer::GetPerFrameConstantBufferGPUVirtualAddress());

		// Does not set per pass constant buffer view as this data is not used by the shadow map pass

		// Set viewport
		D3D12_VIEWPORT shadowMapViewport = {};
		shadowMapViewport.Width = Renderer::SHADOW_MAP_DIMS.x;
		shadowMapViewport.Height = Renderer::SHADOW_MAP_DIMS.y;
		shadowMapViewport.TopLeftX = 0.0f;
		shadowMapViewport.TopLeftY = 0.0f;
		shadowMapViewport.MinDepth = 0.0f;
		shadowMapViewport.MaxDepth = 1.0f;

		D3D12_RECT shadowMapScissor = {};
		shadowMapScissor.top = 0;
		shadowMapScissor.left = 0;
		shadowMapScissor.right = static_cast<LONG>(Renderer::SHADOW_MAP_DIMS.x);
		shadowMapScissor.bottom = static_cast<LONG>(Renderer::SHADOW_MAP_DIMS.y);

		Renderer::Commands::SetViewport(shadowMapViewport, shadowMapScissor);

		// Set only depth target
		Renderer::Commands::SetBackBufferRenderTargets(pSwapChain, true, pSwapChain->GetShadowMapDSDescriptorHandle());

		// Clear only depth target
		Renderer::Commands::ClearRenderTargets(pSwapChain, true, pSwapChain->GetShadowMapDSDescriptorHandle());

		// Submit draw calls
		// Draw scene into shadow map
		demoScene->SetDrawProbes(false);
		demoScene->Draw(0);

		// Copy shadow map depth buffer to shadow map buffer resource
		Renderer::Commands::CopyDepthTargetToResource(shadowMapDepthStencilBuffer.Get(), shadowMapBufferResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// Raytrace global illumination probe field
		// Check raytracing is enabled
		static float GIGatherRateSeconds = 0.1f;
		static bool dispatchRays = true;
		if (dispatchRays)
		{
			// Check if enough time has elapsed since last GI gather
			std::chrono::duration<float, std::milli> GITime = currentTime - lastGIGatherTime;
			if (GITime.count() >= (GIGatherRateSeconds * 1000.0f))
			{
				// Store time that this gather is happening on
				lastGIGatherTime = currentTime;

				// Rebuild acceleration structures
				Renderer::Commands::RebuildTlas(demoScene->GetTlas());

				// Describe dispatch rays
				D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
				dispatchRaysDesc.Width = 1;
				dispatchRaysDesc.Height = 1;
				dispatchRaysDesc.Depth = 1;

				dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = shaderTable->GetGPUVirtualAddress();
				dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = rayGenShaderRecordSize;

				dispatchRaysDesc.MissShaderTable.StartAddress = shaderTable->GetGPUVirtualAddress() + rayGenShaderRecordSize;
				dispatchRaysDesc.MissShaderTable.StrideInBytes = missShaderRecordSize;
				dispatchRaysDesc.MissShaderTable.SizeInBytes = missShaderRecordSize;

				dispatchRaysDesc.HitGroupTable.StartAddress = shaderTable->GetGPUVirtualAddress() + rayGenShaderRecordSize + ALIGN_TO(dispatchRaysDesc.MissShaderTable.SizeInBytes, 64);
				dispatchRaysDesc.HitGroupTable.StrideInBytes = hitGroupShaderRecordSize;
				dispatchRaysDesc.HitGroupTable.SizeInBytes = hitGroupShaderRecordSize;

				// Dispatch rays
				Renderer::Commands::Raytrace(dispatchRaysDesc, raytracingPipelineStateObject.Get(), raytraceOutputResource.Get(), raytraceOutput2Resource.Get());
			}
		}
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Render scene color and depth pass applying direct lighting and diffuse GI
		++passIndex;

		// Update per pass constants
		static const auto& camera = demoScene->GetMainCamera();
		Renderer::Commands::UpdatePerPassConstants(passIndex, glm::vec2(pSwapChain->GetViewportWidth(), pSwapChain->GetViewportHeight()), camera);

		// Set graphics pipeline
		Renderer::Commands::SetGraphicsPipeline(graphicsPipeline.get());

		// Set per frame constant buffer view for pipeline
		Renderer::Commands::SetGraphicsConstantBufferViewRootParam(1, Renderer::GetPerFrameConstantBufferGPUVirtualAddress());

		// Set per pass constant buffer view for pipeline
		Renderer::Commands::SetGraphicsConstantBufferViewRootParam(2,
			Renderer::GetPerPassConstantBufferGPUVirtualAddress() + (Renderer::GetConstantBufferAllignmentSize() * passIndex));

		// Set descriptor table pointer for pipeline
		Renderer::Commands::SetGraphicsDescriptorTableRootParam(3, Renderer::SHADOW_MAP_SRV_DESCRIPTOR_INDEX);

		// Set pixel shader per frame constant buffer view for pipeline
		Renderer::Commands::SetGraphicsConstantBufferViewRootParam(4, Renderer::GetPerFrameConstantBufferGPUVirtualAddress());

		// Set viewport
		Renderer::Commands::SetViewport(pSwapChain);

		// Set render targets
		Renderer::Commands::SetBackBufferRenderTargets(pSwapChain, false, pSwapChain->GetDSDescriptorHandle());

		// Clear render targets
		Renderer::Commands::ClearRenderTargets(pSwapChain, false, pSwapChain->GetDSDescriptorHandle());

		// Update material constants
		static const auto* pMaterials = demoScene->GetMaterialsPtr();
		Renderer::Commands::UpdateMaterialConstants(pMaterials, static_cast<uint32_t>(demoScene->GetMaterialCount()));
		// Do not set any graphics root constant buffer view here yet as the material buffer is not used by the rasterizer, only the raytracer

		// Submit draw calls
		// Draw scene
		static bool visualizeProbeVolume = false;
		demoScene->SetDrawProbes(visualizeProbeVolume);
		demoScene->Draw(0);

		// Copy backbuffer to scene color shader resource
		Renderer::Commands::CopyRenderTargetToResource(pSwapChain, sceneBufferResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		// Copy depth buffer to scene depth shader resource
		Renderer::Commands::CopyDepthTargetToResource(pSwapChain->GetDepthStencilBuffer(), sceneDepthBufferResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Render screen pass
		++passIndex;

		// Does not set per pass constant buffer view as this data is not used by the screen pass
		// Does not set per frame constant buffer view as this data is not used by the screen pass

		// Set graphics pipeline
		Renderer::Commands::SetGraphicsPipeline(screenPassPipeline.get());

		// Set descriptor table pointer for pipeline
		Renderer::Commands::SetGraphicsDescriptorTableRootParam(0, Renderer::SCENE_SRV_DESCRIPTOR_INDEX);

		// Draw screen quad mesh
		Renderer::Commands::SubmitScreenMesh(*screenMesh.get());

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Begin immediate mode GUI for the frame
		Renderer::Commands::BeginImGui();

		// Submit ImGui calls
		// Performance stats window
		static bool displayPerformanceStatsWindow = false;
		if (displayPerformanceStatsWindow)
		{
			ImGui::SetNextWindowSize(ImVec2(200.0f, 50.0f));
			ImGui::SetNextWindowPos(ImVec2(50.0f, 975.0f));
			ImGui::Begin("Perf stats", NULL,
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoDecoration);

			ImGui::Text(("Frametime (ms): " + std::to_string(frameTimeF)).c_str()); // Use ImGui::TextColored to change the text color and add contrast

			ImGui::End();
		}

		// Raytrace output texture view
		static bool showIrradianceRaytraceOutput = false;
		if (showIrradianceRaytraceOutput)
		{
			ImGui::Begin("Irradiance probe texture", &showIrradianceRaytraceOutput, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
			static float zoom = 10.0f;
			ImGui::DragFloat("Zoom", &zoom);
			if (zoom < 1.0f) zoom = 1.0f;
			ImGui::Image((void*)Renderer::GetShaderVisibleDescriptorHeap()->
				GetGPUDescriptorHandle(Renderer::RAYTRACE_IRRADIANCE_SRV_DESCRIPTOR_INDEX).ptr, 
				ImVec2(Renderer::RAYTRACE_IRRADIANCE_OUTPUT_DIMS.x * zoom, Renderer::RAYTRACE_IRRADIANCE_OUTPUT_DIMS.y * zoom));
			ImGui::End();
		}

		static bool showVisibilityRaytraceOutput = false;
		if (showVisibilityRaytraceOutput)
		{
			ImGui::Begin("Visibility probe texture", &showVisibilityRaytraceOutput, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
			static float zoom = 7.0f;
			ImGui::DragFloat("Zoom", &zoom);
			if (zoom < 1.0f) zoom = 1.0f;
			ImGui::Image((void*)Renderer::GetShaderVisibleDescriptorHeap()->
				GetGPUDescriptorHandle(Renderer::RAYTRACE_VISIBILITY_SRV_DESCRIPTOR_INDEX).ptr, 
				ImVec2(Renderer::RAYTRACE_VISIBILITY_OUTPUT_DIMS.x * zoom, Renderer::RAYTRACE_VISIBILITY_OUTPUT_DIMS.y * zoom));
			ImGui::End();
		}

		// Main menu bar
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				Window::Close();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			ImGui::Text("Global illumination");
			ImGui::Separator();
			ImGui::InputFloat("Probe update rate (s)", &GIGatherRateSeconds);
			ImGui::Checkbox("Enable raytracing", &dispatchRays);
			ImGui::Separator();

			ImGui::Text("Light");
			ImGui::Separator();
			ImGui::DragFloat3("Light direction", &demoScene->GetLightDirectionWS().x, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("Light intensity", &demoScene->GetLightIntensity(), 0.01f, 0.01f, 10.0f);
			ImGui::Separator();

			ImGui::Text("Debug");
			ImGui::Separator();
			ImGui::Checkbox("Show irradiance probe texture", &showIrradianceRaytraceOutput);
			ImGui::Checkbox("Show visibility probe texture", &showVisibilityRaytraceOutput);
			ImGui::Checkbox("Visualize probe volume", &visualizeProbeVolume);
			ImGui::DragFloat3("Probe volume position", &demoScene->GetProbeVolumePositionWS().x, 0.1f);
			probeVolume.Update();
			ImGui::Separator();

			ImGui::Text("Stats");
			ImGui::Separator();
			ImGui::Checkbox("Show performance stats", &displayPerformanceStatsWindow);
			ImGui::Separator();

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		// Draw scene ImGui
		demoScene->DrawImGui();

		// End ImGui for the frame
		Renderer::Commands::EndImGui();

		// End the frame for the swap chain
		Renderer::Commands::EndFrame(pSwapChain);

		// Present the frame
		if (!swapChain->Present(Renderer::GetVSyncEnabled()))
		{
			assert(false && "ERROR: Failed to present the swap chain.");
		}
	}

	// Shutdown the renderer
	if (!Renderer::Shutdown())
	{
		assert(false && "Failed to shutdown renderer.");
	}

#ifdef _DEBUG
	ReleaseConsole();
#endif

	return 0;
}