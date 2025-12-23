#pragma once

class DXCBlob
{
public:
	size_t BufferLength() const { return Blob->GetBufferSize(); }
	const void* BufferPointer() const { return Blob->GetBufferPointer(); }

	IDxcBlob** GetAddressOf() { return &Blob; }

private:
	Microsoft::WRL::ComPtr<IDxcBlob> Blob;
};