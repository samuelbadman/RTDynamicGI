#include "Pch.h"
#include "DXCHelper.h"

void DXCHelper::CompileShaderFile(LPCWSTR filepath, LPCWSTR entryPoint, LPCWSTR target, std::unique_ptr<DXCBlob>& blob)
{
	blob = std::make_unique<DXCBlob>();

	Microsoft::WRL::ComPtr<IDxcLibrary> library;
	HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	assert(SUCCEEDED(hr) && "Failed to create DXC library instance.");

	Microsoft::WRL::ComPtr<IDxcCompiler> compiler;
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	assert(SUCCEEDED(hr) && "Failed to create DXC compiler instance.");

	uint32_t codePage = CP_UTF8;
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
	hr = library->CreateBlobFromFile(filepath, &codePage, &sourceBlob);
	assert(SUCCEEDED(hr) && "Failed to create DXC blob from shader file.");

	Microsoft::WRL::ComPtr<IDxcOperationResult> result;
	hr = compiler->Compile(
		sourceBlob.Get(),
		filepath,
		entryPoint,
		target,
		NULL, 0,
		NULL, 0,
		NULL,
		&result
	);
	result->GetStatus(&hr);
	if (FAILED(hr))
	{
		if (result)
		{
			Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorsBlob;
			HRESULT hr = result->GetErrorBuffer(&errorsBlob);
			if (SUCCEEDED(hr) && errorsBlob)
			{
				std::wcout << (const char*)errorsBlob->GetBufferPointer() << std::endl;
			}
			assert(false && "Shader compilation error: Check console output for more information.");
		}
	}
	else
	{
		assert(SUCCEEDED(hr));
		result->GetResult(blob->GetAddressOf());
	}
}
