#pragma once

#include "Vulkan.hpp"

namespace cb::gfx
{

class VulkanPipelineLayout final
{
public:
	VulkanPipelineLayout(VulkanDevice& in_device, 
		VkPipelineLayout in_pipeline_layout) : device(in_device), pipeline_layout(in_pipeline_layout) {}

	VulkanPipelineLayout(VulkanPipelineLayout&& in_other) noexcept = delete;
	
	~VulkanPipelineLayout()
	{
		vkDestroyPipelineLayout(device.get_device(), pipeline_layout, nullptr);	
	}
private:
	VulkanDevice& device;
	VkPipelineLayout pipeline_layout;
};

}