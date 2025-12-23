#pragma once

// Windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Renderer/d3dx12.h"
#include <dxcapi.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

// Imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

// Glm
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "Math/glm/vec2.hpp"
#include "Math/glm/vec3.hpp"
#include "Math/glm/vec4.hpp"
#include "Math/glm/mat3x3.hpp"
#include "Math/glm/mat4x4.hpp"
#include "Math/glm/ext/matrix_transform.hpp"
#include "Math/glm/ext/matrix_clip_space.hpp"
#include "Math/glm/gtc/quaternion.hpp"
#include "Math/glm/gtx/euler_angles.hpp"

// Standard
#include <iostream>
#include <functional>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>

// Macros
#ifdef _DEBUG
#define DEBUG_LOG(x) std::cout << x << "\n";
#else
#define DEBUG_LOG(x)
#endif