#include "engine/gfx/VulkanPipelineLayout.hpp"
#include "engine/gfx/VulkanBuffer.hpp"
#include "engine/gfx/VulkanTexture.hpp"
#include "engine/gfx/VulkanTextureView.hpp"

namespace cb::gfx
{

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& in_device, 
	VkPipelineLayout in_pipeline_layout,
	const std::vector<VkDescriptorSetLayout>& in_set_layouts) : device(in_device), pipeline_layout(in_pipeline_layout),
	set_layouts(in_set_layouts)
{

}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
	size_t i = 0;
	for(const auto& set_layout : set_layouts)
	{
		vkDestroyDescriptorSetLayout(device.get_device(), set_layout, nullptr);
		device.free_descriptor_set_allocator(allocator_indices[i]);
		i++;
	}

	vkDestroyPipelineLayout(device.get_device(), pipeline_layout, nullptr);	
}

}