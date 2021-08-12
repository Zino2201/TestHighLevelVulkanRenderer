#pragma once

#include "engine/Core.hpp"

#if CB_PLATFORM(WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#endif
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "VkBootstrap.h"
#include "engine/gfx/Memory.hpp"
#include "engine/gfx/DeviceResource.hpp"
#include "engine/gfx/Result.hpp"
#include "engine/gfx/Shader.hpp"
#include "engine/gfx/Format.hpp"
#include <atomic>
#include "engine/logger/Logger.hpp"

namespace cb::gfx
{

/** Debug counter used to track undeleted resources */
inline static std::atomic_size_t alive_vulkan_objects = 0;

CB_DEFINE_LOG_CATEGORY(vulkan);

template<typename T>
struct VulkanResourcePtr
{
	T* ptr;

	VulkanResourcePtr(T* in_ptr) : ptr(in_ptr) {}

	T* operator->() const
	{
		return ptr;
	}

	BackendDeviceResource get() const
	{
		return reinterpret_cast<uint64_t>(ptr);
	}
	
	operator BackendDeviceResource() const
	{
		return reinterpret_cast<uint64_t>(ptr);
	}
};
	
template<typename T, typename... Args>
VulkanResourcePtr<T> new_resource(Args&&... in_args)
{
	alive_vulkan_objects++;
	return VulkanResourcePtr(new T(std::forward<Args>(in_args)...));
}

template<typename T, typename... Args>
VulkanResourcePtr<T> new_resource_no_count(Args&&... in_args)
{
	return VulkanResourcePtr(new T(std::forward<Args>(in_args)...));
}

template<typename T>
T* get_resource(BackendDeviceResource in_resource)
{
	return reinterpret_cast<T*>(in_resource);
}

template<typename T>
void free_resource(BackendDeviceResource in_resource)
{
	delete reinterpret_cast<T*>(in_resource);
	alive_vulkan_objects--;
}

/**
 * Utils functions to convert common types
 */
inline Result convert_result(VkResult in_result)
{
	switch(in_result)
	{
	case VK_SUCCESS:
		return Result::Success;
	case VK_TIMEOUT:
		return Result::Timeout;
	default:
	case VK_ERROR_UNKNOWN:
		return Result::ErrorUnknown;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return Result::ErrorOutOfDeviceMemory;
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return Result::ErrorOutOfHostMemory;
	case VK_ERROR_INITIALIZATION_FAILED:
		return Result::ErrorInitializationFailed;
	}
}

inline VmaMemoryUsage convert_memory_usage(MemoryUsage in_mem_usage)
{
	switch (in_mem_usage)
	{
	default:
	case MemoryUsage::CpuOnly:
		return VMA_MEMORY_USAGE_CPU_ONLY;
	case MemoryUsage::CpuToGpu:
		return VMA_MEMORY_USAGE_CPU_TO_GPU;
	case MemoryUsage::GpuToCpu:
		return VMA_MEMORY_USAGE_GPU_TO_CPU;
	case MemoryUsage::GpuOnly:
		return VMA_MEMORY_USAGE_GPU_ONLY;
	}
}

inline VkFormat convert_format(const Format& in_format)
{
	switch(in_format)
	{
	default:
	case Format::Undefined:
		return VK_FORMAT_UNDEFINED;

	/** Depth & stencil */
	case Format::D32Sfloat:
		return VK_FORMAT_D32_SFLOAT;
	case Format::D32SfloatS8Uint:
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case Format::D24UnormS8Uint:
		return VK_FORMAT_D24_UNORM_S8_UINT;

	/** RGBA */
	case Format::R8Unorm:
		return VK_FORMAT_R8_UNORM;
	case Format::R8G8B8Unorm:
		return VK_FORMAT_R8G8B8_UNORM;
	case Format::R8G8B8A8Unorm:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case Format::R8G8B8A8Srgb:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case Format::B8G8R8A8Unorm:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case Format::R16G16B16A16Sfloat:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case Format::R32G32Sfloat:
		return VK_FORMAT_R32G32_SFLOAT;
	case Format::R32G32B32Sfloat:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case Format::R32G32B32A32Sfloat:
		return VK_FORMAT_R32G32B32A32_SFLOAT;

	/** Block */
	case Format::Bc1RgbUnormBlock:
		return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
	case Format::Bc1RgbaUnormBlock:
		return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case Format::Bc1RgbSrgbBlock:
		return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
	case Format::Bc1RgbaSrgbBlock:
		return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

	case Format::Bc3UnormBlock:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	case Format::Bc3SrgbBlock:
		return VK_FORMAT_BC3_SRGB_BLOCK;
		
	case Format::Bc5UnormBlock:
		return VK_FORMAT_BC5_UNORM_BLOCK;
	case Format::Bc5SnormBlock:
		return VK_FORMAT_BC5_SNORM_BLOCK;
		
	case Format::Bc6HUfloatBlock:
		return VK_FORMAT_BC6H_UFLOAT_BLOCK;
	case Format::Bc6HSfloatBlock:
		return VK_FORMAT_BC6H_SFLOAT_BLOCK;

	case Format::Bc7UnormBlock:
		return VK_FORMAT_BC7_UNORM_BLOCK;
	case Format::Bc7SrgbBlock:
		return VK_FORMAT_BC7_SRGB_BLOCK;
	}
}

inline Format convert_vk_format(const VkFormat in_format)
{
	switch(in_format)
	{
	default:
	case VK_FORMAT_UNDEFINED:
		return Format::Undefined;

	/** Depth & stencil */
	case VK_FORMAT_D32_SFLOAT:
		return Format::D32Sfloat;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return Format::D32SfloatS8Uint;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		return Format::D24UnormS8Uint;

	/** RGBA */
	case VK_FORMAT_R8_UNORM:
		return Format::R8Unorm;
	case VK_FORMAT_R8G8B8_UNORM:
		return Format::R8G8B8Unorm;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return Format::R8G8B8A8Unorm;
	case VK_FORMAT_R8G8B8A8_SRGB:
		return Format::R8G8B8A8Srgb;
	case VK_FORMAT_B8G8R8A8_UNORM:
		return Format::B8G8R8A8Unorm;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return Format::R16G16B16A16Sfloat;
	case VK_FORMAT_R32G32_SFLOAT:
		return Format::R32G32Sfloat;
	case VK_FORMAT_R32G32B32_SFLOAT:
		return Format::R32G32B32Sfloat;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return Format::R32G32B32A32Sfloat;

	/** Block */
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		return Format::Bc1RgbUnormBlock;
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		return Format::Bc1RgbaUnormBlock;
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		return Format::Bc1RgbSrgbBlock;
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		return Format::Bc1RgbaSrgbBlock;

	case VK_FORMAT_BC3_UNORM_BLOCK:
		return Format::Bc3UnormBlock;
	case VK_FORMAT_BC3_SRGB_BLOCK:
		return Format::Bc3SrgbBlock;
		
	case VK_FORMAT_BC5_UNORM_BLOCK:
		return Format::Bc5UnormBlock;
	case VK_FORMAT_BC5_SNORM_BLOCK:
		return Format::Bc5SnormBlock;
		
	case VK_FORMAT_BC6H_UFLOAT_BLOCK:
		return Format::Bc6HUfloatBlock;
	case VK_FORMAT_BC6H_SFLOAT_BLOCK:
		return Format::Bc6HSfloatBlock;

	case VK_FORMAT_BC7_UNORM_BLOCK:
		return Format::Bc7UnormBlock;
	case VK_FORMAT_BC7_SRGB_BLOCK:
		return Format::Bc7SrgbBlock;
	}
}

inline VkObjectType convert_object_type(DeviceResourceType in_type)
{
	switch(in_type)
	{
	case DeviceResourceType::Buffer:
		return VK_OBJECT_TYPE_BUFFER;
	case DeviceResourceType::Texture:
		return VK_OBJECT_TYPE_IMAGE;
	case DeviceResourceType::TextureView:
		return VK_OBJECT_TYPE_IMAGE_VIEW;
	case DeviceResourceType::Sampler:
		return VK_OBJECT_TYPE_SAMPLER;
	case DeviceResourceType::Swapchain:
		return VK_OBJECT_TYPE_SWAPCHAIN_KHR;
	case DeviceResourceType::CommandPool:
		return VK_OBJECT_TYPE_COMMAND_POOL;
	case DeviceResourceType::CommandList:
		return VK_OBJECT_TYPE_COMMAND_BUFFER;
	case DeviceResourceType::Fence:
		return VK_OBJECT_TYPE_FENCE;
	case DeviceResourceType::Pipeline:
		return VK_OBJECT_TYPE_PIPELINE;
	case DeviceResourceType::PipelineLayout:
		return VK_OBJECT_TYPE_PIPELINE_LAYOUT;
	case DeviceResourceType::Semaphore:
		return VK_OBJECT_TYPE_SEMAPHORE;
	case DeviceResourceType::Shader:
		return VK_OBJECT_TYPE_SHADER_MODULE;
	default:
		return VK_OBJECT_TYPE_UNKNOWN;
	}
}

}