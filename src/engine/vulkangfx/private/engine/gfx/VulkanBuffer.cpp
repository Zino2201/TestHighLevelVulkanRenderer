#include "engine/gfx/VulkanBuffer.hpp"
#include "VulkanDevice.hpp"

namespace cb::gfx
{

VulkanBuffer::VulkanBuffer(VulkanDevice& in_device,
	const VkBuffer& in_buffer,
	const VmaAllocation& in_allocation,
	const VmaAllocationInfo& in_alloc_info) : device(in_device), buffer(in_buffer), allocation(in_allocation),
	alloc_info(in_alloc_info) {}
	
VulkanBuffer::~VulkanBuffer()
{
	vmaDestroyBuffer(device.get_allocator(), buffer, allocation);
}
	
}
