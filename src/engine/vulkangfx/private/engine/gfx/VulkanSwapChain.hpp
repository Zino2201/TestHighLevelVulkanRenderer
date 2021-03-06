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
		const vkb::Swapchain& in_swapchain);
	~VulkanSwapChain();

	[[nodiscard]] VkResult acquire_image(VkSemaphore in_signal_semaphore);
	void present(const std::span<VkSemaphore>& in_wait_semaphores);

	[[nodiscard]] const BackendDeviceResource& get_texture_view() const { return image_views[current_image]; }
	[[nodiscard]] const std::vector<BackendDeviceResource>& get_textures() const { return images; }
	[[nodiscard]] const std::vector<BackendDeviceResource>& get_texture_views() const { return image_views; }
	[[nodiscard]] Format get_format() const { return convert_vk_format(swapchain.image_format); }
	[[nodiscard]] uint32_t get_current_image_idx() const { return current_image; }
	[[nodiscard]] const vkb::Swapchain& get_swapchain() const { return swapchain; }
private:
	VulkanDevice& device;
	vkb::Swapchain swapchain;
	VkSurfaceKHR surface;
	uint32_t current_image;
	std::vector<BackendDeviceResource> images;
	std::vector<BackendDeviceResource> image_views;
};
	
}