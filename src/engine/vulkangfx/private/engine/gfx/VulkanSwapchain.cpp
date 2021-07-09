#include "engine/gfx/VulkanSwapChain.hpp"
#include "engine/gfx/VulkanDevice.hpp"
#include "engine/gfx/VulkanTexture.hpp"
#include "engine/gfx/VulkanTextureView.hpp"

namespace cb::gfx
{

VulkanSwapChain::VulkanSwapChain(VulkanDevice& in_device,
	VkSurfaceKHR in_surface) : device(in_device), surface(in_surface), current_image(0) {}

VulkanSwapChain::~VulkanSwapChain()
{
	for(const auto& image : images)
		free_resource<VulkanTexture>(image);

	for(const auto& image_view : image_views)
		free_resource<VulkanTextureView>(image_view);
	
	if(swapchain.swapchain)
		vkb::destroy_swapchain(swapchain);
}
	
VkResult VulkanSwapChain::create(const uint32_t in_width, 
	const uint32_t in_height)
{
	/** Move the old swapchain handle if it exists, it allows on some implementation to recreate a swapchain faster */
	vkb::Swapchain old_swapchain = swapchain;

	current_image = 0;
	images.clear();

	for(const auto& view : image_views)
		device.destroy_texture_view(view);
	
	image_views.clear();
	
	vkb::SwapchainBuilder swapchain_builder(device.get_physical_device(),
		device.get_device(),
		surface);

	VkSurfaceFormatKHR format = {};
	
	format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	format.format = VK_FORMAT_B8G8R8A8_UNORM;
	
	auto result = 
		swapchain_builder.set_old_swapchain(old_swapchain.swapchain)
		.set_desired_extent(in_width, in_height)
		.set_desired_format(format)
		.build();
	if(!result)
	{
		spdlog::error("Failed to create Vulkan swapchain: {}", result.error().message());
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	swapchain = result.value();
	
	if(old_swapchain.swapchain)
		vkb::destroy_swapchain(swapchain);

	/** Set image/image_views */
	auto image_list = swapchain.get_images();
	auto image_view_list = swapchain.get_image_views();

	for(const auto& image : image_list.value())
	{
		images.push_back(new_resource<VulkanTexture>(device, image));
	}
	
	for(const auto& image_view : image_view_list.value())
	{
		image_views.push_back(new_resource<VulkanTextureView>(device, image_view));
	}
		
	return VK_SUCCESS;
}

VkResult VulkanSwapChain::acquire_image(VkSemaphore in_signal_semaphore)
{
	return vkAcquireNextImageKHR(device.get_device(),
		swapchain.swapchain,
		std::numeric_limits<uint64_t>::max(),
		in_signal_semaphore,
		VK_NULL_HANDLE,
		&current_image);
}

void VulkanSwapChain::present(const std::span<VkSemaphore>& in_wait_semaphores)
{
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain.swapchain;
	info.waitSemaphoreCount = static_cast<uint32_t>(in_wait_semaphores.size());
	info.pWaitSemaphores = in_wait_semaphores.data();
	info.pResults = nullptr;
	info.pImageIndices = &current_image;
	
	vkQueuePresentKHR(device.get_present_queue(),
		&info);	
}
	
}