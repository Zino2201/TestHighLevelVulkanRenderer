#pragma once

#include "engine/gfx/SwapChain.hpp"

namespace cb::gfx
{

class VulkanDevice;

class VulkanSwapChain final
{
public:
	VulkanSwapChain(VulkanDevice& in_device,
		VkSurfaceKHR in_surface);
	~VulkanSwapChain();

	VkResult create(const uint32_t in_width, 
		const uint32_t in_height);
private:

private:
	VulkanDevice& device;
	vkb::Swapchain swapchain;
	VkSurfaceKHR surface;
	uint32_t image_count;
	uint32_t current_image;
	uint32_t current_frame;
	std::vector<BackendDeviceResource> images;
	std::vector<BackendDeviceResource> image_views;
};
	
}