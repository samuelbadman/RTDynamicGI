#pragma once
#include "DXCBlob.h"

namespace DXCHelper
{
	void CompileShaderFile(LPCWSTR filepath, LPCWSTR entryPoint, LPCWSTR target, std::unique_ptr<DXCBlob>& blob);
}