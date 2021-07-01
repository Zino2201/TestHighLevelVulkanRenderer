#pragma once

#include "engine/gfx/BackendDevice.hpp"
#include "Vulkan.hpp"
#include "engine/gfx/VulkanBackend.hpp"
#include <robin_hood.h>

namespace cb::gfx
{

class VulkanSwapChain;
	
class VulkanDevice final : public BackendDevice
{
	struct DeviceWrapper
	{
		vkb::Device device;

		explicit DeviceWrapper(vkb::Device&& in_device) : device(in_device) {}
		~DeviceWrapper() { vkb::destroy_device(device); }
	};
	
	/** Small facility to manage VkSurfaceKHRs */
	class SurfaceManager
	{
		struct SurfaceWrapper
		{
			VulkanDevice& device;
			VkSurfaceKHR surface;

			SurfaceWrapper(VulkanDevice& in_device,
				VkSurfaceKHR in_surface) : device(in_device), surface(in_surface) {}

			SurfaceWrapper(SurfaceWrapper&& in_other) noexcept :
				device(in_other.device),
				surface(std::exchange(in_other.surface, VK_NULL_HANDLE)) {}
			
			~SurfaceWrapper()
			{
				vkDestroySurfaceKHR(device.get_backend().get_instance(), surface, nullptr);
			}
		};
	public:
		SurfaceManager(VulkanDevice& in_device) : device(in_device) {}

		cb::Result<VkSurfaceKHR, Result> get_or_create(void* in_os_handle)
		{
			{
				if(auto it = surfaces.find(in_os_handle); it != surfaces.end())
					return it->second.surface;
			}

			auto surface = create_surface(in_os_handle);
			if(!surface)
			{
				spdlog::error("Failed to create Vulkan surface: {}", surface.get_error());
				return make_error(Result::ErrorInitializationFailed);
			}
			
			surfaces.insert({ in_os_handle, SurfaceWrapper(device, surface.get_value()) });
			return make_result(surface.get_value());
		}
	private:
		cb::Result<VkSurfaceKHR, VkResult> create_surface(void* in_os_handle)
		{
#if CB_PLATFORM(WINDOWS)
			VkWin32SurfaceCreateInfoKHR create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			create_info.hinstance = nullptr;
			create_info.hwnd = static_cast<HWND>(in_os_handle);
			create_info.flags = 0;
			create_info.pNext = nullptr;

			VkSurfaceKHR handle = VK_NULL_HANDLE;
			if(VkResult result = vkCreateWin32SurfaceKHR(device.get_backend().get_instance(),
				&create_info,
				nullptr,
				&handle); result != VK_SUCCESS)
				return make_error(result);

			return make_result(handle);
#endif
		}
	private:
		VulkanDevice& device;
		robin_hood::unordered_map<void*, SurfaceWrapper> surfaces;
	};
public:
	explicit VulkanDevice(VulkanBackend& in_backend, vkb::Device&& in_device);
	~VulkanDevice() override;

	cb::Result<BackendDeviceResource, Result> create_buffer(const BufferCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_swap_chain(const SwapChainCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_shader(const ShaderCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_render_pass(const RenderPassCreateInfo& in_create_info) override;
	
	void destroy_buffer(const BackendDeviceResource& in_buffer) override;
	void destroy_swap_chain(const BackendDeviceResource& in_swap_chain) override;
	void destroy_shader(const BackendDeviceResource& in_shader) override;
	void destroy_gfx_pipeline(const BackendDeviceResource& in_shader) override;
	void destroy_render_pass(const BackendDeviceResource& in_shader) override;

	cb::Result<void*, Result> map_buffer(const BackendDeviceResource& in_buffer) override;
	void unmap_buffer(const BackendDeviceResource& in_buffer) override;

	[[nodiscard]] VulkanBackend& get_backend() const { return backend; }
	[[nodiscard]] VkDevice get_device() const { return device_wrapper.device.device; }
	[[nodiscard]] VkPhysicalDevice get_physical_device() const { return device_wrapper.device.physical_device.physical_device; }
	[[nodiscard]] VmaAllocator get_allocator() const { return allocator; }
private:
	VulkanBackend& backend;
	VmaAllocator allocator;
	DeviceWrapper device_wrapper;
	SurfaceManager surface_manager;
};
	
}