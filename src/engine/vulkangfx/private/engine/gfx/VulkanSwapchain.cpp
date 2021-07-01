#include "engine/gfx/VulkanSwapChain.hpp"
#include "engine/gfx/VulkanDevice.hpp"

namespace cb::gfx
{

VulkanSwapChain::VulkanSwapChain(VulkanDevice& in_device,
	VkSurfaceKHR in_surface) : device(in_device), surface(in_surface),
		image_count(0), current_image(0), current_frame(0) {}

VulkanSwapChain::~VulkanSwapChain()
{
	if(swapchain.swapchain)
		vkb::destroy_swapchain(swapchain);
}
	
VkResult VulkanSwapChain::create(const uint32_t in_width, 
	const uint32_t in_height)
{
	/** Move the old swapchain handle if it exists, it allows on some implementation to recreate a swapchain faster */
	vkb::Swapchain old_swapchain = swapchain;

	current_image = 0;
	current_frame = 0;
	images.clear();
	image_views.clear();

	vkb::SwapchainBuilder swapchain_builder(device.get_physical_device(),
		device.get_device(),
		surface);
	auto result = 
		swapchain_builder.set_old_swapchain(old_swapchain.swapchain)
		.build();
	if(!result)
	{
		spdlog::error("Failed to create Vulkan swapchain: {}", result.error().message());
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	swapchain = result.value();
	
	if(old_swapchain.swapchain)
		vkb::destroy_swapchain(swapchain);

	return VK_SUCCESS;
}
	
}