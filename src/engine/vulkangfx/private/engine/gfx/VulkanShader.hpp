#pragma once

#include "Vulkan.hpp"

namespace cb::gfx
{

struct VulkanShader
{
	VulkanDevice& device;
	VkShaderModule shader_module;

	VulkanShader(VulkanDevice& in_device, VkShaderModule in_shader_module)
		: device(in_device), shader_module(in_shader_module) {}
	
	~VulkanShader()
	{
		vkDestroyShaderModule(device.get_device(), shader_module, nullptr);
	}

	VulkanShader(const VulkanShader&) = delete;
	void operator=(const VulkanShader&) = delete;
	
	VulkanShader(VulkanShader&& in_other) noexcept : device(in_other.device),
		shader_module(std::exchange(shader_module, VK_NULL_HANDLE)) {}
};
	
}