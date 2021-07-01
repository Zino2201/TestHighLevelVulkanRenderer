#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanShader.hpp"

namespace cb::gfx
{

VulkanDevice::VulkanDevice(VulkanBackend& in_backend, vkb::Device&& in_device) :
	backend(in_backend),
	device_wrapper(DeviceWrapper(std::move(in_device))),
	surface_manager(*this),
	allocator(nullptr)
{
	VmaAllocatorCreateInfo create_info = {};
	create_info.instance = backend.get_instance();
	create_info.device = device_wrapper.device.device;
	create_info.physicalDevice = device_wrapper.device.physical_device.physical_device;

	// TODO: Error check
	vmaCreateAllocator(&create_info, &allocator);
}
	
VulkanDevice::~VulkanDevice() = default;

cb::Result<BackendDeviceResource, Result> VulkanDevice::create_buffer(const BufferCreateInfo& in_create_info)
{
	CB_CHECK(in_create_info.size != 0 && in_create_info.usage_flags != BufferUsageFlags())
	if(in_create_info.size == 0 || in_create_info.usage_flags == BufferUsageFlags())
		return make_error(Result::ErrorInvalidParameter);

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = nullptr;
	buffer_create_info.size = in_create_info.size;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = nullptr;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.usage = 0;
	buffer_create_info.flags = 0;

	/** Usage flags */
	if(in_create_info.usage_flags & BufferUsageFlagBits::VertexBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::IndexBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::UniformBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::StorageBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	
	VmaAllocationCreateInfo alloc_create_info = {};
	alloc_create_info.flags = 0;
	alloc_create_info.usage = convert_memory_usage(in_create_info.mem_usage);
	
	VmaAllocation allocation;
	VkBuffer handle;
	
	VkResult result = vmaCreateBuffer(allocator, 
		&buffer_create_info, 
		&alloc_create_info,
		&handle,
		&allocation,
		nullptr);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto buffer = new_resource<VulkanBuffer>(*this, handle, allocation);
	return make_result(buffer.get());
}
	
cb::Result<BackendDeviceResource, Result> VulkanDevice::create_swap_chain(const SwapChainCreateInfo& in_create_info)
{
	CB_CHECK(in_create_info.os_handle && in_create_info.width != 0 && in_create_info.height != 0);
	if(!in_create_info.os_handle || in_create_info.width == 0 || in_create_info.height == 0)
		return make_error(Result::ErrorInvalidParameter);

	auto surface = surface_manager.get_or_create(in_create_info.os_handle);
	if(!surface)
	{
		return make_error(surface.get_error());
	}
	
	auto swapchain = new_resource<VulkanSwapChain>(*this, surface.get_value());
	VkResult result = swapchain->create(in_create_info.width, in_create_info.height);
	if(result != VK_SUCCESS)
	{
		free_resource<VulkanSwapChain>(swapchain);
		return make_error(convert_result(result));
	}
	
	return static_cast<BackendDeviceResource>(swapchain);
}

cb::Result<BackendDeviceResource, Result> VulkanDevice::create_shader(const ShaderCreateInfo& in_create_info)
{
	CB_CHECK(!in_create_info.bytecode.empty());
	if(in_create_info.bytecode.empty())
		return make_error(Result::ErrorInvalidParameter);

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.codeSize = in_create_info.bytecode.size();
	create_info.pCode = in_create_info.bytecode.data();
	create_info.flags = 0;

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(get_device(),
		&create_info,
		nullptr,
		&shader_module);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto shader = new_resource<VulkanShader>(*this, shader_module);
	return make_result(shader.get());
}

cb::Result<BackendDeviceResource, Result> VulkanDevice::create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info)
{
	s
}
	
cb::Result<BackendDeviceResource, Result> VulkanDevice::create_render_pass(const RenderPassCreateInfo& in_create_info)
{
	
}

void VulkanDevice::destroy_buffer(const BackendDeviceResource& in_buffer)
{
	free_resource<VulkanBuffer>(in_buffer);
}

void VulkanDevice::destroy_swap_chain(const BackendDeviceResource& in_swap_chain)
{
	free_resource<VulkanSwapChain>(in_swap_chain);
}

void VulkanDevice::destroy_shader(const BackendDeviceResource& in_shader)
{
	free_resource<VulkanShader>(in_shader);
}

void VulkanDevice::destroy_gfx_pipeline(const BackendDeviceResource& in_shader)
{
	
}
	
void VulkanDevice::destroy_render_pass(const BackendDeviceResource& in_shader)
{
	
}

/** Buffers */
cb::Result<void*, Result> VulkanDevice::map_buffer(const BackendDeviceResource& in_buffer)
{
	void* data = nullptr;

	auto buffer = get_resource<VulkanBuffer>(in_buffer);
	VkResult result = vmaMapMemory(allocator,
		buffer->get_allocation(),
		&data);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	return make_result(data);
}

void VulkanDevice::unmap_buffer(const BackendDeviceResource& in_buffer)
{
	auto buffer = get_resource<VulkanBuffer>(in_buffer);
	vmaUnmapMemory(allocator, buffer->get_allocation());
}
	
}