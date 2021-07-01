#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

namespace cb::gfx
{

VulkanDevice::~VulkanDevice() = default;

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
		return make_error(vk_result_to_result(result));
	}
	
	return static_cast<BackendDeviceResource>(swapchain);
}

void VulkanDevice::destroy_swap_chain(const BackendDeviceResource& in_swap_chain)
{
	free_resource<VulkanSwapChain>(in_swap_chain);
}
	
}