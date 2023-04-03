#pragma once

#include <Windows.h>
#include <winrt/base.h>

// These macros mess with some of Vulkan's functions
#undef CreateEvent
#undef CreateSemaphore

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

#define VKROOTS_NEGOTIATION_INTERFACE VRLayer_NegotiateLoaderLayerInterfaceVersion

#include "vkroots.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D12

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <type_traits>

#include "utils/logger.h"