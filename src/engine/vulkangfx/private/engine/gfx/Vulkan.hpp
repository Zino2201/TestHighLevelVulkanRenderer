#pragma once

#if CB_PLATFORM(WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#include "VkBootstrap.h"

namespace cb::gfx
{

/** Debug counter used to track undeleted resources */
size_t alive_vulkan_objects = 0;

template<typename T>
struct VulkanResourcePtr
{
	T* ptr;

	VulkanResourcePtr(T* in_ptr) : ptr(in_ptr) {}

	T* operator->() const
	{
		return ptr;
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
void free_resource(BackendDeviceResource in_resource)
{
	delete reinterpret_cast<T*>(in_resource);
	alive_vulkan_objects--;
}

inline Result vk_result_to_result(VkResult in_result)
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

}