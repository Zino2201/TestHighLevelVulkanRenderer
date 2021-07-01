#pragma once

#include "engine/gfx/Vulkan.hpp"
#include "engine/gfx/Buffer.hpp"

namespace cb::gfx
{

class VulkanDevice;

class VulkanBuffer final
{
public:
	VulkanBuffer(VulkanDevice& in_device,
		const VkBuffer& in_buffer,
		const VmaAllocation& in_allocation);
	~VulkanBuffer();

	[[nodiscard]] VkBuffer get_buffer() const { return buffer; }
	[[nodiscard]] VmaAllocation get_allocation() const { return allocation; }
private:
	VulkanDevice& device;
	VkBuffer buffer;
	VmaAllocation allocation;
};
	
}