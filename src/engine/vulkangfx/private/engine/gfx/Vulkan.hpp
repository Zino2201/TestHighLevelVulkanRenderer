#pragma once

#include "engine/Core.hpp"

#if CB_PLATFORM(WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#endif
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "VkBootstrap.h"
#include "engine/gfx/DeviceResource.hpp"
#include "engine/gfx/Result.hpp"
#include "engine/gfx/Memory.hpp"

namespace cb::gfx
{

/** Debug counter used to track undeleted resources */
inline static size_t alive_vulkan_objects = 0;

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

}