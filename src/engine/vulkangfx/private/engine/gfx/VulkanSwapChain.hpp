#pragma once

#include "engine/gfx/Vulkan.hpp"
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

	[[nodiscard]] VkResult acquire_image(VkSemaphore in_signal_semaphore);
	void present(const std::span<VkSemaphore>& in_wait_semaphores);

	[[nodiscard]] const BackendDeviceResource& get_texture_view() const { return image_views[current_image]; }
private:
	VulkanDevice& device;
	vkb::Swapchain swapchain;
	VkSurfaceKHR surface;
	uint32_t current_image;
	std::vector<BackendDeviceResource> images;
	std::vector<BackendDeviceResource> image_views;
};
	
}