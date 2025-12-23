#include "Pch.h"
#include "Renderer.h"
#include "Math/Math.h"
#include "Math/Transform.h"
#include "Window/Window.h"

#include "Pipeline/GraphicsPipeline.h"
#include "DescriptorHeap.h"
#include "Material.h"

constexpr float CLEAR_COLOR[4] = { 0.005f, 0.005f, 0.005f, 1.0f };
constexpr UINT64 CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES = 256;
constexpr uint32_t SIZE_64KB = 65536;
constexpr size_t BACK_BUFFER_COUNT = 3;
constexpr uint32_t MAX_DRAWS_PER_FRAME = SIZE_64KB / CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES;

// Renderer
Microsoft::WRL::ComPtr<IDXGIFactory4> DXGIFactory;
bool TearingSupported;
Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter;
DXGI_ADAPTER_DESC1 AdapterDesc;
Microsoft::WRL::ComPtr<ID3D12Device5> Device;
UINT RTDescriptorIncrementSize;
UINT DSDescriptorIncrementSize;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> DirectCommandQueue;
std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, BACK_BUFFER_COUNT> DirectCommandAllocators;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> DirectCommandList;
std::array<Microsoft::WRL::ComPtr<ID3D12Fence>, BACK_BUFFER_COUNT> FrameFences;
std::array<UINT64, BACK_BUFFER_COUNT> FrameFenceValues;
HANDLE MainThreadFenceEvent;
bool VSyncEnabled = true;
std::unique_ptr<DescriptorHeap> CBVSRVUAVDescriptorHeap;

// Asset loading command objects
Microsoft::WRL::ComPtr<ID3D12CommandQueue> GraphicsLoadCommandQueue;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GraphicsLoadCommandAllocator;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GraphicsLoadCommandList;
Microsoft::WRL::ComPtr<ID3D12Fence> GraphicsLoadFence;
UINT64 GraphicsLoadFenceValue = 0;

// Constant buffers
struct PerObjectConstants
{
    glm::mat4 WorldMatrix = glm::identity<glm::mat4>();
    glm::vec4 Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 NormalMatrix = glm::identity<glm::mat4>();
    uint32_t Lit = true;
};

struct PerFrameConstants
{
    glm::mat4 LightMatrix = glm::identity<glm::mat4>();
    glm::vec4 ProbePositionsWS[Renderer::MAX_PROBE_COUNT];
    glm::vec4 LightDirectionWS = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 PackedData = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // Stores probe count (x), probe spacing (y), light intensity (z)
};

struct PerPassConstants
{
    glm::mat4 ViewMatrix = glm::identity<glm::mat4>();
    glm::mat4 ProjectionMatrix = glm::identity<glm::mat4>();
    glm::vec4 CameraPositionWS = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
};

struct MaterialConstants
{
    glm::vec4 Colors[Renderer::MAX_MATERIAL_COUNT];
};

Microsoft::WRL::ComPtr<ID3D12Resource> PerFrameConstantBuffer;
uint8_t* MappedPerFrameConstantBufferLocation;

Microsoft::WRL::ComPtr<ID3D12Resource> PerObjectConstantBuffer;
uint8_t* MappedPerObjectConstantBufferLocation;

Microsoft::WRL::ComPtr<ID3D12Resource> PerPassConstantBuffer;
uint8_t* MappedPerPassConstantBufferLocation;

Microsoft::WRL::ComPtr<ID3D12Resource> MaterialConstantBuffer;
uint8_t* MappedMaterialConstantBufferLocation;

// Rendering
size_t FrameIndex = 0;
uint32_t FrameDrawCount = 0;

// ImGui


bool EnableDebugLayer()
{
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
    {
        return false;
    }
    debugController->EnableDebugLayer();

    // Enable GPU based validation and synchronized command queue validation
    //Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
    //if (FAILED(debugController.As(&debugController1)))
    //{
    //    return false;
    //}
    //debugController1->SetEnableGPUBasedValidation(TRUE);
    //debugController1->SetEnableSynchronizedCommandQueueValidation(FALSE);

    return true;
#endif // _DEBUG
}

bool ReportLiveObjects()
{
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
    if (FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
        return false;
    }
    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    return true;
#endif
}

bool CreateFactory(Microsoft::WRL::ComPtr<IDXGIFactory4>& factory)
{
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

    return SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
}

bool CheckTearingSupport(Microsoft::WRL::ComPtr<IDXGIFactory4> factory)
{
    BOOL allowTearing = false;

    Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(factory.As(&factory5)))
    {
        if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        {
            allowTearing = false;
        }
    }

    return allowTearing == TRUE;
}

bool SupportsRaytracing(Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter)
{
    Microsoft::WRL::ComPtr<ID3D12Device> testDevice;
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
    return SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
        && SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
        && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

bool GetAdapter(Microsoft::WRL::ComPtr<IDXGIFactory4> factory, Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter, DXGI_ADAPTER_DESC1& adapterDesc)
{
    // Select a hardware adapter, supporting D3D12 device creation, raytracing and favouring the adapter with the largest dedicated video memory
    Microsoft::WRL::ComPtr<IDXGIAdapter1> intermediateAdapter;
    SIZE_T maxDedicatedVideoMemory = 0;
    for (UINT i = 0; factory->EnumAdapters1(i, &intermediateAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 intermediateAdapterDesc;
        intermediateAdapter->GetDesc1(&intermediateAdapterDesc);

        if ((intermediateAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            (SUCCEEDED(D3D12CreateDevice(intermediateAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                SupportsRaytracing(intermediateAdapter) &&
                intermediateAdapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory))
        {
            maxDedicatedVideoMemory = intermediateAdapterDesc.DedicatedVideoMemory;
            if (FAILED(intermediateAdapter.As(&adapter)))
            {
                return false;
            }
            adapterDesc = intermediateAdapterDesc;
        }
    }

    return true;
}

bool CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter, Microsoft::WRL::ComPtr<ID3D12Device5>& device)
{
    auto hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

    // Setup debug info if in debug configuration
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device.As(&infoQueue)))
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        D3D12_MESSAGE_SEVERITY severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        D3D12_MESSAGE_ID denyIDs[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer errors on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };

        D3D12_INFO_QUEUE_FILTER newFilter = {};
        newFilter.DenyList.NumSeverities = _countof(severities);
        newFilter.DenyList.pSeverityList = severities;
        newFilter.DenyList.NumIDs = _countof(denyIDs);
        newFilter.DenyList.pIDList = denyIDs;
        if (FAILED(infoQueue->PushStorageFilter(&newFilter)))
        {
            return false;
        }
    }
#endif // _DEBUG

    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

bool CreateCommandQueue(const D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    return SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
}

bool CreateCommandAllocator(const D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device> device,
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& commandAllocator)
{
    return SUCCEEDED(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
}

bool CreateCommandList(const D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device> device,
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>& commandList)
{
    return SUCCEEDED(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
}

bool CreateFence(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT64 initialValue, D3D12_FENCE_FLAGS fenceFlags, Microsoft::WRL::ComPtr<ID3D12Fence>& fence)
{
    return SUCCEEDED(device->CreateFence(initialValue, fenceFlags, IID_PPV_ARGS(&fence)));
}

bool CreateFenceEvent(HANDLE& fenceEvent)
{
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return fenceEvent != nullptr;
}

bool WaitForFenceToReachValue(Microsoft::WRL::ComPtr<ID3D12Fence> fence, UINT64 value, HANDLE event, DWORD duration)
{
    if (fence->GetCompletedValue() < value)
    {
        if (FAILED(fence->SetEventOnCompletion(value, event)))
        {
            return false;
        }

        ::WaitForSingleObject(event, duration);
    }

    return true;
}

bool Renderer::Init(const uint32_t shaderVisibleCBVSRVUAVDescriptorCount)
{
    // Enable debug features if in debug configuration
#ifdef _DEBUG
    if (!EnableDebugLayer())
    {
        DEBUG_LOG("ERROR: Failed to enable debug layer.");
        return false;
    }

    if (!ReportLiveObjects())
    {
        DEBUG_LOG("ERROR: ReportLiveObjects() failed.");
        return false;
    }
#endif

    // Create DXGI factory
    if (!CreateFactory(DXGIFactory))
    {
        DEBUG_LOG("ERROR: Failed to create DXGI factory.");
        return false;
    }

    // Check tearing support
    TearingSupported = CheckTearingSupport(DXGIFactory);

    // Get a hardware adapter
    if (!GetAdapter(DXGIFactory, Adapter, AdapterDesc))
    {
        DEBUG_LOG("ERROR: Failed to get a hardware adapter.");
        return false;
    }

    // Create D3D12 device
    if (!CreateDevice(Adapter, Device))
    {
        DEBUG_LOG("ERROR: Failed to create D3D12 device.");
        return false;
    }

    // Get RTV increment size
    RTDescriptorIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Get DSV increment size
    DSDescriptorIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // Create a direct command queue
    if (!CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, DirectCommandQueue))
    {
        DEBUG_LOG("ERROR: Failed to create direct command queue.");
        return false;
    }

    // Create direct command allocators
    for (auto& allocator : DirectCommandAllocators)
    {
        if (!CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, allocator))
        {
            DEBUG_LOG("ERROR: Failed to create direct command allocator.");
            return false;
        }
    }

    // Create a direct command list
    if (!CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, DirectCommandAllocators[0], DirectCommandList))
    {
        DEBUG_LOG("ERROR: Failed to create command list");
        return false;
    }

    // Close the command list as it will not be recording now
    if (FAILED(DirectCommandList->Close()))
    {
        DEBUG_LOG("ERROR: Failed to close direct command list.");
        return false;
    }

    // Create frame fences
    for (auto& fence : FrameFences)
    {
        if (!CreateFence(Device, 0, D3D12_FENCE_FLAG_NONE, fence))
        {
            DEBUG_LOG("ERROR: Failed to create fence.");
            return false;
        }
    }

    // Create main thread OS event
    if (!CreateFenceEvent(MainThreadFenceEvent))
    {
        DEBUG_LOG("ERROR: Failed to create OS event handle for main thread fence.");
        return false;
    }

    // Create copy resources
    if (!CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, GraphicsLoadCommandQueue))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load command queue.");
        return false;
    }

    if (!CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, GraphicsLoadCommandAllocator))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load command allocator.");
        return false;
    }

    if (!CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, GraphicsLoadCommandAllocator, GraphicsLoadCommandList))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load command list.");
        return false;
    }

    if (FAILED(GraphicsLoadCommandList->Close()))
    {
        DEBUG_LOG("ERROR: Failed to close graphics load command list.");
        return false;
    }

    if (!CreateFence(Device, GraphicsLoadFenceValue, D3D12_FENCE_FLAG_NONE, GraphicsLoadFence))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load fence.");
        return false;
    }

    // Create per frame constant buffer
    auto perFrameHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto perFrameResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(SIZE_64KB);

    if (FAILED(Device->CreateCommittedResource(&perFrameHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &perFrameResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&PerFrameConstantBuffer))))
    {
        DEBUG_LOG("ERROR: Failed to create per frame constant buffer.");
        return false;
    }

    // Set a debug name for the resource
    if (FAILED(PerFrameConstantBuffer->SetName(L"PerFrameConstantBuffer")))
    {
        DEBUG_LOG("ERROR: Failed to name per frame constant buffer.");
        return false;
    }

    // Map the per frame constant buffer
    D3D12_RANGE perFrameReadRange(0, 0);
    void* mappedPerFrameConstantBufferResource;
    if FAILED(PerFrameConstantBuffer->Map(0, &perFrameReadRange, &mappedPerFrameConstantBufferResource))
    {
        DEBUG_LOG("ERROR: Failed to map per frame constant buffer.");
        return false;
    }
    MappedPerFrameConstantBufferLocation = static_cast<uint8_t*>(mappedPerFrameConstantBufferResource);

    // Create per object constant buffer
    auto perObjectHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto perObjectResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(SIZE_64KB * 2);

    if (FAILED(Device->CreateCommittedResource(&perObjectHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &perObjectResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&PerObjectConstantBuffer))))
    {
        DEBUG_LOG("ERROR: Failed to create per object constant buffer.");
        return false;
    }

    // Set a debug name for the resource
    if (FAILED(PerFrameConstantBuffer->SetName(L"PerObjectConstantBuffer")))
    {
        DEBUG_LOG("ERROR: Failed to name per object constant buffer.");
        return false;
    }

    // Map the per object constant buffer
    D3D12_RANGE perObjectReadRange(0, 0);
    void* mappedPerObjectConstantBufferResource;
    if FAILED(PerObjectConstantBuffer->Map(0, &perObjectReadRange, &mappedPerObjectConstantBufferResource))
    {
        DEBUG_LOG("ERROR: Failed to map per frame constant buffer.");
        return false;
    }
    MappedPerObjectConstantBufferLocation = static_cast<uint8_t*>(mappedPerObjectConstantBufferResource);

    // Create material buffer
    auto materialHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto materialResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(SIZE_64KB);

    if (FAILED(Device->CreateCommittedResource(&materialHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &materialResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&MaterialConstantBuffer))))
    {
        DEBUG_LOG("ERROR: Failed to create material constant buffer.");
        return false;
    }

    // Set a debug name for the material buffer
    if (FAILED(MaterialConstantBuffer->SetName(L"MaterialConstantBuffer")))
    {
        DEBUG_LOG("ERROR: Failed to name material constant buffer.");
        return false;
    }

    // Map the material buffer
    D3D12_RANGE materialReadRange(0, 0);
    void* mappedMaterialBufferResource;
    if FAILED(MaterialConstantBuffer->Map(0, &materialReadRange, &mappedMaterialBufferResource))
    {
        DEBUG_LOG("ERROR: Failed to map material constant buffer.");
        return false;
    }
    MappedMaterialConstantBufferLocation = static_cast<uint8_t*>(mappedMaterialBufferResource);

    // Create per pass buffer
    auto perPassHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto perPassResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(SIZE_64KB);

    if (FAILED(Device->CreateCommittedResource(&perPassHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &perPassResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&PerPassConstantBuffer))))
    {
        DEBUG_LOG("ERROR: Failed to create per pass constant buffer.");
        return false;
    }

    // Set a debug name for the per pass buffer
    if (FAILED(PerPassConstantBuffer->SetName(L"PerPassConstantBuffer")))
    {
        DEBUG_LOG("ERROR: Failed to name per pass constant buffer.");
        return false;
    }

    // Map the per pass buffer
    D3D12_RANGE perPassReadRange(0, 0);
    void* mappedPerPassBufferResource;
    if FAILED(PerPassConstantBuffer->Map(0, &perPassReadRange, &mappedPerPassBufferResource))
    {
        DEBUG_LOG("ERROR: Failed to map material constant buffer.");
        return false;
    }
    MappedPerPassConstantBufferLocation = static_cast<uint8_t*>(mappedPerPassBufferResource);

    // Initialize shader visible descriptor heap
    CBVSRVUAVDescriptorHeap = std::make_unique<DescriptorHeap>();
    CBVSRVUAVDescriptorHeap->Init(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 
        shaderVisibleCBVSRVUAVDescriptorCount, true);

    // Initialize imgui
    assert(shaderVisibleCBVSRVUAVDescriptorCount > 0 && "At least 1 shader visible CBV SRV UAV descriptor is required for ImGui resources.");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(Window::GetHandle());
    ImGui_ImplDX12_Init(Device.Get(), BACK_BUFFER_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM, CBVSRVUAVDescriptorHeap->Get(),
        CBVSRVUAVDescriptorHeap->GetCPUDescriptorHandle(0),
        CBVSRVUAVDescriptorHeap->GetGPUDescriptorHandle(0));

	return true;
}

bool Renderer::Shutdown()
{
    // Wait for all queues to finish executing work
    if (!Flush())
    {
        return false;
    }

    // Close main thread fence event handle
    if (::CloseHandle(MainThreadFenceEvent) == 0)
    {
        return false;
    }

    // Shutdown imgui
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    return true;
}

bool Renderer::Flush()
{
    size_t i = 0;
    for (const auto& fence : FrameFences)
    {
        if (FAILED(DirectCommandQueue->Signal(fence.Get(), ++FrameFenceValues[i])))
        {
            return false;
        }

        if (!WaitForFenceToReachValue(fence, FrameFenceValues[i], MainThreadFenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count())))
        {
            return false;
        }

        ++i;
    }

    return true;
}

bool Renderer::CreateSwapChain(HWND windowHandle, UINT width, UINT height, DXGI_FORMAT format, std::unique_ptr<SwapChain>& swapChain)
{
    auto temp = std::make_unique<SwapChain>();

    if (!temp->Init(DXGIFactory, DirectCommandQueue, Device, windowHandle, width, height, BACK_BUFFER_COUNT,
        TearingSupported, RTDescriptorIncrementSize, format))
    {
        return false;
    }

    swapChain = std::move(temp);

    return true;
}

bool Renderer::ResizeSwapChain(SwapChain* pSwapChain, UINT newWidth, UINT newHeight)
{
    return pSwapChain->Resize(Device, newWidth, newHeight, RTDescriptorIncrementSize);
}

template<>
bool Renderer::CreateGraphicsPipeline<Renderer::GraphicsPipeline>(SwapChain* pSwapChain, std::unique_ptr<Renderer::GraphicsPipelineBase>& pipeline)
{
    auto temp = std::make_unique<Renderer::GraphicsPipeline>();
    if (!temp->Init(Device.Get(), pSwapChain->GetFormat()))
    {
        return false;
    }
    pipeline = std::move(temp);
    return true;
}

template<>
bool Renderer::CreateGraphicsPipeline<Renderer::ScreenPassPipeline>(SwapChain* pSwapChain, std::unique_ptr<Renderer::GraphicsPipelineBase>& pipeline)
{
    auto temp = std::make_unique<Renderer::ScreenPassPipeline>();
    if (!temp->Init(Device.Get(), pSwapChain->GetFormat()))
    {
        return false;
    }
    pipeline = std::move(temp);
    return true;
}

template<>
bool Renderer::CreateGraphicsPipeline<Renderer::ShadowMapPassPipeline>(SwapChain* pSwapChain, std::unique_ptr<Renderer::GraphicsPipelineBase>& pipeline)
{
    auto temp = std::make_unique<Renderer::ShadowMapPassPipeline>();
    if (!temp->Init(Device.Get(), pSwapChain->GetFormat()))
    {
        return false;
    }
    pipeline = std::move(temp);
    return true;
}

void Renderer::CreateStagedMesh(const std::vector<Vertex1Pos1UV1Norm>& vertices, const std::vector<uint32_t>& indices,
    const std::wstring& name, std::unique_ptr<Mesh>& mesh)
{
    mesh = std::make_unique<Mesh>(Device.Get(), vertices, indices, name);
}

bool Renderer::LoadStagedMeshesOntoGPU(std::unique_ptr<Mesh>* pMeshes, const size_t meshCount)
{
    auto CreateIntermediateUploadBuffer = [](const size_t bufferSize, const void* bufferData, 
        Microsoft::WRL::ComPtr<ID3D12Resource>& intermediateBuffer, const std::wstring& name)
    {
        // Create intermediate vertex upload buffer
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        if (FAILED(Device->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateBuffer)))
            )
        {
            assert(false && "Failed to create intermediate mesh buffer.");
        }

        // Set a debug name for the resource
        if (FAILED(intermediateBuffer->SetName(name.c_str())))
        {
            assert(false && "Failed to set name for intermediate mesh buffer.");
        }

        // Map the intermediate buffer resource
        D3D12_RANGE readRange(0, 0);
        void* intermediateResourceStart;
        if (FAILED(intermediateBuffer->Map(0, &readRange, &intermediateResourceStart)))
        {
            assert(false && "Failed to create intermediate mesh buffer.");
        }

        // Copy data into the intermediate upload buffer
        memcpy(intermediateResourceStart, bufferData, bufferSize);

        // Unmap intermediate buffer resource
        intermediateBuffer->Unmap(0, nullptr);
    };

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateUploadBuffers(meshCount * 2);

    // For each mesh
    for (size_t i = 0, j = 0; i < meshCount; ++i, j += 2)
    {
        // Create and fill intermediate upload buffers with vertex and index data
        // Intermediate buffers are stored next to each other for each mesh
        // [[inter vertex buffer mesh 0][inter index buffer mesh 0][inter vertex buffer mesh 1][inter index buffer mesh 1]...]
        auto* pMesh = pMeshes[i].get();
        auto& intermediateVertexUploadBuffer = intermediateUploadBuffers[j];
        auto& intermediateIndexUploadBuffer = intermediateUploadBuffers[j + 1];

        CreateIntermediateUploadBuffer(pMesh->GetRequiredBufferWidthVertexBuffer(), pMesh->GetVerticesData(), 
            intermediateVertexUploadBuffer, L"VerticesIntermediateUploadBuffer" + std::to_wstring(i));

        CreateIntermediateUploadBuffer(pMesh->GetRequiredBufferWidthIndexBuffer(), pMesh->GetIndicesData(),
            intermediateIndexUploadBuffer, L"IndexIntermediateUploadBuffer" + std::to_wstring(i));
    }

    if (FAILED(GraphicsLoadCommandAllocator->Reset()))
    {
        DEBUG_LOG("Failed to reset copy command allocator.");
        return false;
    }

    if (FAILED(GraphicsLoadCommandList->Reset(GraphicsLoadCommandAllocator.Get(), nullptr)))
    {
        DEBUG_LOG("Failed to reset copy command list.");
        return false;
    }

    std::vector<CD3DX12_RESOURCE_BARRIER> transitionBarriers(meshCount * 2);

    // For each mesh
    for (size_t i = 0, j = 0; i < meshCount; ++i, j += 2)
    {
        auto* pMesh = pMeshes[i].get();
        auto* pMeshVertexBuffer = pMesh->GetVertexBuffer();
        auto* pMeshIndexBuffer = pMesh->GetIndexBuffer();

        GraphicsLoadCommandList->CopyResource(pMeshVertexBuffer, intermediateUploadBuffers[j].Get());
        GraphicsLoadCommandList->CopyResource(pMeshIndexBuffer, intermediateUploadBuffers[j + 1].Get());

        transitionBarriers[j] = CD3DX12_RESOURCE_BARRIER::Transition(pMeshVertexBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        transitionBarriers[j + 1] = CD3DX12_RESOURCE_BARRIER::Transition(pMeshIndexBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }

    GraphicsLoadCommandList->ResourceBarrier(static_cast<UINT>(transitionBarriers.size()), transitionBarriers.data());

    if (FAILED(GraphicsLoadCommandList->Close()))
    {
        return false;
    }

    ID3D12CommandList* commandLists[] = { GraphicsLoadCommandList.Get() };
    GraphicsLoadCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ++GraphicsLoadFenceValue;
    if (FAILED(GraphicsLoadCommandQueue->Signal(GraphicsLoadFence.Get(), GraphicsLoadFenceValue)))
    {
        return false;
    }

    // Wait on CPU for graphics load queue to finish
    return WaitForFenceToReachValue(GraphicsLoadFence, GraphicsLoadFenceValue, MainThreadFenceEvent,
        static_cast<DWORD>(std::chrono::milliseconds::max().count()));
}

void Renderer::CreateBottomLevelAccelerationStructure(Mesh& mesh, std::unique_ptr<BottomLevelAccelerationStructure>& blas)
{
    blas = std::make_unique<BottomLevelAccelerationStructure>(Device.Get(), mesh);
}

bool Renderer::BuildBottomLevelAccelerationStructures(std::unique_ptr<BottomLevelAccelerationStructure>* pStructures, const size_t structureCount)
{
    if (FAILED(GraphicsLoadCommandAllocator->Reset()))
    {
        DEBUG_LOG("Failed to reset copy command allocator.");
        return false;
    }

    if (FAILED(GraphicsLoadCommandList->Reset(GraphicsLoadCommandAllocator.Get(), nullptr)))
    {
        DEBUG_LOG("Failed to reset copy command list.");
        return false;
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers(structureCount);

    for (size_t i = 0; i < structureCount; ++i)
    {
        auto& structure = pStructures[i];
        GraphicsLoadCommandList->BuildRaytracingAccelerationStructure(&structure->GetBuildDesc(), 0, nullptr);
        barriers[i] = CD3DX12_RESOURCE_BARRIER::UAV(structure->GetBlas());
    }

    GraphicsLoadCommandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    if (FAILED(GraphicsLoadCommandList->Close()))
    {
        return false;
    }

    ID3D12CommandList* commandLists[] = { GraphicsLoadCommandList.Get() };
    GraphicsLoadCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ++GraphicsLoadFenceValue;
    if (FAILED(GraphicsLoadCommandQueue->Signal(GraphicsLoadFence.Get(), GraphicsLoadFenceValue)))
    {
        return false;
    }

    // Wait on CPU for graphics load queue to finish
    return WaitForFenceToReachValue(GraphicsLoadFence, GraphicsLoadFenceValue, MainThreadFenceEvent,
        static_cast<DWORD>(std::chrono::milliseconds::max().count()));
}

void Renderer::CreateTopLevelAccelerationStructure(std::unique_ptr<TopLevelAccelerationStructure>& tlas, const bool allowUpdate, const uint32_t instanceCount)
{
    tlas = std::make_unique<TopLevelAccelerationStructure>(Device.Get(), allowUpdate, instanceCount);
}

bool Renderer::BuildTopLevelAccelerationStructures(std::unique_ptr<TopLevelAccelerationStructure>* pStructures, const size_t structureCount)
{
    if (FAILED(GraphicsLoadCommandAllocator->Reset()))
    {
        DEBUG_LOG("Failed to reset copy command allocator.");
        return false;
    }

    if (FAILED(GraphicsLoadCommandList->Reset(GraphicsLoadCommandAllocator.Get(), nullptr)))
    {
        DEBUG_LOG("Failed to reset copy command list.");
        return false;
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers(structureCount);

    for (size_t i = 0; i < structureCount; ++i)
    {
        auto& structure = pStructures[i];
        GraphicsLoadCommandList->BuildRaytracingAccelerationStructure(&structure->GetBuildDesc(), 0, nullptr);
        barriers[i] = CD3DX12_RESOURCE_BARRIER::UAV(structure->GetTlasResource());
    }

    GraphicsLoadCommandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    if (FAILED(GraphicsLoadCommandList->Close()))
    {
        return false;
    }

    ID3D12CommandList* commandLists[] = { GraphicsLoadCommandList.Get() };
    GraphicsLoadCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ++GraphicsLoadFenceValue;
    if (FAILED(GraphicsLoadCommandQueue->Signal(GraphicsLoadFence.Get(), GraphicsLoadFenceValue)))
    {
        return false;
    }

    // Wait on CPU for graphics load queue to finish
    return WaitForFenceToReachValue(GraphicsLoadFence, GraphicsLoadFenceValue, MainThreadFenceEvent,
        static_cast<DWORD>(std::chrono::milliseconds::max().count()));
}

void Renderer::AddSRVDescriptorToShaderVisibleHeap(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, const uint32_t descriptorIndex)
{
    assert(descriptorIndex != 0 && "Descriptor index 0 is occupied by ImGui resources in CBV SRV UAV descriptor heap. Use another index.");
    Device->CreateShaderResourceView(pResource, pDesc, CBVSRVUAVDescriptorHeap->GetCPUDescriptorHandle(descriptorIndex));
}

void Renderer::AddUAVDescriptorToShaderVisibleHeap(ID3D12Resource* pResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, const uint32_t descriptorIndex)
{
    assert(descriptorIndex != 0 && "Descriptor index 0 is occupied by ImGui resources in CBV SRV UAV descriptor heap. Use another index.");
    Device->CreateUnorderedAccessView(pResource, nullptr, pDesc, CBVSRVUAVDescriptorHeap->GetCPUDescriptorHandle(descriptorIndex));
}

UINT Renderer::GetRTDescriptorIncrementSize()
{
    return RTDescriptorIncrementSize;
}

UINT Renderer::GetDSDescriptorIncrementSize()
{
    return DSDescriptorIncrementSize;
}

bool Renderer::GetVSyncEnabled()
{
    return VSyncEnabled;
}

void Renderer::SetVSyncEnabled(const bool enabled)
{
    VSyncEnabled = enabled;
}

const DescriptorHeap* Renderer::GetShaderVisibleDescriptorHeap()
{
    return CBVSRVUAVDescriptorHeap.get();
}

D3D12_GPU_VIRTUAL_ADDRESS Renderer::GetPerFrameConstantBufferGPUVirtualAddress()
{
    return PerFrameConstantBuffer->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS Renderer::GetPerObjectConstantBufferGPUVirtualAddress()
{
    return PerObjectConstantBuffer->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS Renderer::GetPerPassConstantBufferGPUVirtualAddress()
{
    return PerPassConstantBuffer->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS Renderer::GetMaterialConstantBufferGPUVirtualAddress()
{
    return MaterialConstantBuffer->GetGPUVirtualAddress();
}

UINT64 Renderer::GetConstantBufferAllignmentSize()
{
    return CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES;
}

ID3D12Device5* Renderer::GetDevice()
{
    return Device.Get();
}

// Commands
bool Renderer::Commands::StartFrame(SwapChain* pSwapChain)
{
    // Get current frame resources
    FrameIndex = pSwapChain->GetCurrentBackBufferIndex();
    auto* pCurrentFrameCommandAllocator = DirectCommandAllocators[FrameIndex].Get();
    auto& frameFenceValue = FrameFenceValues[FrameIndex];

    // Wait for previous frame
    if (!WaitForFenceToReachValue(FrameFences[FrameIndex], frameFenceValue, MainThreadFenceEvent,
        static_cast<DWORD>(std::chrono::milliseconds::max().count())))
    {
        return false;
    }

    // Increment frame fence value for the next frame
    ++frameFenceValue;

    // Reset command recording objects
    if (FAILED(pCurrentFrameCommandAllocator->Reset()))
    {
        return false;
    }

    if (FAILED(DirectCommandList->Reset(pCurrentFrameCommandAllocator, nullptr)))
    {
        return false;
    }

    // Start recording commands into direct command list
    // Transition current frame render target from present state to render target state
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetBackBuffers()[FrameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    DirectCommandList->ResourceBarrier(1, &barrier);

    return true;
}

bool Renderer::Commands::EndFrame(SwapChain* pSwapChain)
{
    // Transition current frame render target from render target state to present state
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetBackBuffers()[FrameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    DirectCommandList->ResourceBarrier(1, &barrier);

    // Stop recording commands into direct command lists
    if (FAILED(DirectCommandList->Close()))
    {
        return false;
    }

    // Execute recorded command lists
    ID3D12CommandList* commandListsToExecute[] = { DirectCommandList.Get() };
    DirectCommandQueue->ExecuteCommandLists(_countof(commandListsToExecute), commandListsToExecute);

    FrameDrawCount = 0;

    return SUCCEEDED(DirectCommandQueue->Signal(FrameFences[FrameIndex].Get(), FrameFenceValues[FrameIndex]));
}

void Renderer::Commands::ClearRenderTargets(SwapChain* pSwapChain, bool depthOnly, const CD3DX12_CPU_DESCRIPTOR_HANDLE& depthTargetDescriptorHandle)
{
    if (depthOnly)
    {
        DirectCommandList->ClearDepthStencilView(depthTargetDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }
    else
    {
        auto rtvHandle = pSwapChain->GetRTDescriptorHandleForFrame(FrameIndex);
        DirectCommandList->ClearRenderTargetView(rtvHandle, CLEAR_COLOR, 0, nullptr);
        DirectCommandList->ClearDepthStencilView(depthTargetDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }
}

void Renderer::Commands::SetBackBufferRenderTargets(SwapChain* pSwapChain, bool depthOnly, const CD3DX12_CPU_DESCRIPTOR_HANDLE& depthTargetDescriptorHandle)
{
    if (depthOnly)
    {
        DirectCommandList->OMSetRenderTargets(0, nullptr, FALSE, &depthTargetDescriptorHandle);
    }
    else
    {
        auto rtvHandle = pSwapChain->GetRTDescriptorHandleForFrame(FrameIndex);
        DirectCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthTargetDescriptorHandle);
    }
}

void Renderer::Commands::SetPrimitiveTopology()
{
    DirectCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Renderer::Commands::SetViewport(SwapChain* pSwapChain)
{
    DirectCommandList->RSSetViewports(1, &pSwapChain->GetViewport());
    DirectCommandList->RSSetScissorRects(1, &pSwapChain->GetScissorRect());
}

void Renderer::Commands::SetViewport(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect)
{
    DirectCommandList->RSSetViewports(1, &viewport);
    DirectCommandList->RSSetScissorRects(1, &scissorRect);
}

void Renderer::Commands::SetGraphicsPipeline(GraphicsPipelineBase* pPipeline)
{
    DirectCommandList->SetPipelineState(pPipeline->GetPipelineState());
    DirectCommandList->SetGraphicsRootSignature(pPipeline->GetRootSignature());
}

void Renderer::Commands::UpdatePerFrameConstants(const std::vector<Transform>& probeTransformsWS, const glm::vec3& lightDirectionWS, const float lightIntensity, const float probeSpacing)
{
    PerFrameConstants perFrameConstants = {};

    // Update probe position
    assert(probeTransformsWS.size() <= Renderer::MAX_PROBE_COUNT && "Attempting to use more probes than the max probe count.");
    size_t index = 0;
    for (const auto& transform : probeTransformsWS)
    {
        perFrameConstants.ProbePositionsWS[index] = glm::vec4(transform.Position.x, transform.Position.y, transform.Position.z, 1.0f);
        ++index;
    }

    // Update probe count, spacing and light intensity
    perFrameConstants.PackedData.x = static_cast<float>(probeTransformsWS.size());
    perFrameConstants.PackedData.y = probeSpacing;
    perFrameConstants.PackedData.z = lightIntensity;

    // Update light direction
    perFrameConstants.LightDirectionWS.x = lightDirectionWS.x;
    perFrameConstants.LightDirectionWS.y = lightDirectionWS.y;
    perFrameConstants.LightDirectionWS.z = lightDirectionWS.z;
    perFrameConstants.LightDirectionWS.w = 1.0f;

    // Update light matrix
    auto lightPosition = glm::normalize(lightDirectionWS) * 7.0f;
    lightPosition.x = -lightPosition.x;
    lightPosition.y = -lightPosition.y;
    perFrameConstants.LightMatrix = Math::CalculateOrthographicProjectionMatrix(10.0f, 10.0f, -10.0f, 10.0f) *
        Math::CalculateViewMatrix(
            lightPosition,
            Math::FindLookAtRotation(lightPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

    memcpy(MappedPerFrameConstantBufferLocation, &perFrameConstants, sizeof(PerFrameConstants));
}

void Renderer::Commands::UpdatePerPassConstants(const uint32_t passIndex, const glm::vec2& viewportDims, const Camera& camera)
{
    PerPassConstants perPassConstants = {};

    // Calculate pass view matrix
    perPassConstants.ViewMatrix = Math::CalculateViewMatrix(camera.Position, camera.Rotation);

    // Calculate pass projection matrix
    switch (camera.Settings.ProjectionMode)
    {
    case Camera::CameraSettings::ProjectionMode::PERSPECTIVE:
        perPassConstants.ProjectionMatrix = Math::CalculatePerspectiveProjectionMatrix(camera.Settings.PerspectiveFOV,
            viewportDims.x,
            viewportDims.y,
            camera.Settings.PerspectiveNearClipPlane,
            camera.Settings.PerspectiveFarClipPlane);
        break;

    case Camera::CameraSettings::ProjectionMode::ORTHOGRAPHIC:
        perPassConstants.ProjectionMatrix = Math::CalculateOrthographicProjectionMatrix(camera.Settings.OrthographicWidth,
            camera.Settings.OrthographicHeight,
            camera.Settings.OrthographicNearClipPlane,
            camera.Settings.OrthographicFarClipPlane);
        break;
    }

    // Update camera world space position
    perPassConstants.CameraPositionWS = glm::vec4(camera.Position.x, camera.Position.y, camera.Position.z, 1.0f);

    assert(passIndex < 256 && "Unsupported pass index is being used in an UpdatePerPassConstants call.");
    auto bufferOffset = passIndex * CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES;
    memcpy(MappedPerPassConstantBufferLocation + bufferOffset, &perPassConstants, sizeof(PerPassConstants));
}

void Renderer::Commands::UpdateMaterialConstants(const Renderer::Material* pMaterials, const uint32_t materialCount)
{
    assert(materialCount <= Renderer::MAX_MATERIAL_COUNT && "Updating an unsupported number of materials.");

    // Update material constants buffer
    MaterialConstants materialConstants = {};

    for (size_t i = 0; i < Renderer::MAX_MATERIAL_COUNT; ++i)
    {
        materialConstants.Colors[i] = pMaterials[i].GetColor();
    }

    memcpy(MappedMaterialConstantBufferLocation, &materialConstants, sizeof(MaterialConstants));
}

void Renderer::Commands::SubmitMesh(UINT perObjectConstantsParameterIndex, const Mesh& mesh, const Transform& transform, const glm::vec4& color, const bool lit)
{
    // Update per object constant buffer
    PerObjectConstants perObjectConstants = {};
    perObjectConstants.WorldMatrix = Math::CalculateWorldMatrix(transform);
    perObjectConstants.Color = color;
    perObjectConstants.Lit = lit;

    glm::mat3 worldMatrix3x3 = perObjectConstants.WorldMatrix;
    perObjectConstants.NormalMatrix = glm::inverse(glm::transpose(worldMatrix3x3));

    auto objectConstantBufferOffset = FrameDrawCount * CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES;
    memcpy(MappedPerObjectConstantBufferLocation + objectConstantBufferOffset, &perObjectConstants, sizeof(PerObjectConstants));

    DirectCommandList->SetGraphicsRootConstantBufferView(perObjectConstantsParameterIndex, PerObjectConstantBuffer->GetGPUVirtualAddress() + objectConstantBufferOffset);
    DirectCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    DirectCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    DirectCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);

    ++FrameDrawCount;
}

void Renderer::Commands::SubmitScreenMesh(const Mesh& mesh)
{
    DirectCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    DirectCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    DirectCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);
}

void Renderer::Commands::SetDescriptorHeaps()
{
    ID3D12DescriptorHeap* heaps[] = { CBVSRVUAVDescriptorHeap->Get() };
    DirectCommandList->SetDescriptorHeaps(_countof(heaps), heaps);
}

void Renderer::Commands::BeginImGui()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Renderer::Commands::EndImGui()
{
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), DirectCommandList.Get());
}

void Renderer::Commands::RebuildTlas(TopLevelAccelerationStructure* tlas)
{
    assert(tlas->UpdateAllowed() && "Attempting to rebuild a tlas that does not allow updating.");

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    inputs.NumDescs = tlas->GetInstanceCount();
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    Device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    auto* pTlas = tlas->GetTlasResource();
    auto tlasGPUVirtualAddress = pTlas->GetGPUVirtualAddress();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    buildDesc.SourceAccelerationStructureData = tlasGPUVirtualAddress;
    buildDesc.Inputs.InstanceDescs = tlas->GetInstancesBuffer()->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = tlasGPUVirtualAddress;
    buildDesc.ScratchAccelerationStructureData = tlas->GetScratchBuffer()->GetGPUVirtualAddress();

    DirectCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(pTlas);
    DirectCommandList->ResourceBarrier(1, &barrier);
}

void Renderer::Commands::Raytrace(const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc, ID3D12StateObject* pPipelineStateObject, ID3D12Resource* pRaytraceOutputResource,
    ID3D12Resource* pRaytraceOutput2Resource)
{
    DirectCommandList->SetPipelineState1(pPipelineStateObject);
    DirectCommandList->DispatchRays(&dispatchRaysDesc);
    CD3DX12_RESOURCE_BARRIER barriers[] = { CD3DX12_RESOURCE_BARRIER::UAV(pRaytraceOutputResource), CD3DX12_RESOURCE_BARRIER::UAV(pRaytraceOutput2Resource) };
    DirectCommandList->ResourceBarrier(_countof(barriers), barriers);
}

void Renderer::Commands::SetGraphicsDescriptorTableRootParam(UINT rootParameterIndex, const uint32_t baseDescriptorIndex)
{
    DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, CBVSRVUAVDescriptorHeap->GetGPUDescriptorHandle(baseDescriptorIndex));
}

void Renderer::Commands::SetGraphicsConstantBufferViewRootParam(UINT rootParameterIndex, const D3D12_GPU_VIRTUAL_ADDRESS bufferAddress)
{
    DirectCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, bufferAddress);
}

void Renderer::Commands::DebugCopyResourceToRenderTarget(SwapChain* pSwapChain, ID3D12Resource* pSrcResource, D3D12_RESOURCE_STATES srcResourceState)
{
    auto* pRenderTargetResource = pSwapChain->GetBackBuffers()[FrameIndex].Get();

    CD3DX12_RESOURCE_BARRIER beginBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargetResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(pSrcResource, srcResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    DirectCommandList->ResourceBarrier(_countof(beginBarriers), beginBarriers);

    DirectCommandList->CopyResource(pRenderTargetResource, pSrcResource);

    CD3DX12_RESOURCE_BARRIER endBarriers[] = {
    CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargetResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET),
    CD3DX12_RESOURCE_BARRIER::Transition(pSrcResource, D3D12_RESOURCE_STATE_COPY_SOURCE, srcResourceState)
    };
    DirectCommandList->ResourceBarrier(_countof(endBarriers), endBarriers);
}

void Renderer::Commands::CopyRenderTargetToResource(SwapChain* pSwapChain, ID3D12Resource* pDstResource, D3D12_RESOURCE_STATES dstResourceState)
{
    auto* pRenderTargetResource = pSwapChain->GetBackBuffers()[FrameIndex].Get();

    CD3DX12_RESOURCE_BARRIER beginBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargetResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(pDstResource, dstResourceState, D3D12_RESOURCE_STATE_COPY_DEST)
    };
    DirectCommandList->ResourceBarrier(_countof(beginBarriers), beginBarriers);

    DirectCommandList->CopyResource(pDstResource, pRenderTargetResource);

    CD3DX12_RESOURCE_BARRIER endBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargetResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(pDstResource, D3D12_RESOURCE_STATE_COPY_DEST, dstResourceState)
    };
    DirectCommandList->ResourceBarrier(_countof(endBarriers), endBarriers);
}

void Renderer::Commands::CopyDepthTargetToResource(ID3D12Resource* pDepthTarget, ID3D12Resource* pDstResource, D3D12_RESOURCE_STATES dstResourceState)
{
    CD3DX12_RESOURCE_BARRIER beginBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(pDepthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(pDstResource, dstResourceState, D3D12_RESOURCE_STATE_COPY_DEST)
    };
    DirectCommandList->ResourceBarrier(_countof(beginBarriers), beginBarriers);

    DirectCommandList->CopyResource(pDstResource, pDepthTarget);

    CD3DX12_RESOURCE_BARRIER endBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(pDepthTarget, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
        CD3DX12_RESOURCE_BARRIER::Transition(pDstResource, D3D12_RESOURCE_STATE_COPY_DEST, dstResourceState)
    };
    DirectCommandList->ResourceBarrier(_countof(endBarriers), endBarriers);
}
