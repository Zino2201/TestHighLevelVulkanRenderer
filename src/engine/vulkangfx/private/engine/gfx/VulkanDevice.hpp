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

	/** Facility to manage framebuffer accouting for lifetimes to destroy unused framebuffers */
	class FramebufferManager
	{
		static constexpr uint8_t framebuffer_expiration_frames = 10;
		
		struct Entry
		{
			/** Frame count since this framebuffer has been polled */
			VulkanDevice& device;
			uint8_t unpolled_frames;
			VkFramebuffer framebuffer;

			Entry(VulkanDevice& in_device, VkFramebuffer in_framebuffer)
				: device(in_device), unpolled_frames(0), framebuffer(in_framebuffer) {}
			~Entry() { vkDestroyFramebuffer(device.get_device(), framebuffer, nullptr); }

			Entry(Entry&& in_other) : device(in_other.device),
				unpolled_frames(std::move(in_other.unpolled_frames)),
				framebuffer(std::exchange(in_other.framebuffer, VK_NULL_HANDLE)) {}
		};

		VulkanDevice& device;
		robin_hood::unordered_map<Framebuffer, Entry> framebuffers;

	public:
		FramebufferManager(VulkanDevice& in_device) : device(in_device) {}
		
		void new_frame();
		VkFramebuffer get_or_create(VkRenderPass in_render_pass, const Framebuffer& in_framebuffer);
	};
public:
	explicit VulkanDevice(VulkanBackend& in_backend, vkb::Device&& in_device);
	~VulkanDevice() override;

	void new_frame() override;

	cb::Result<BackendDeviceResource, Result> create_buffer(const BufferCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_swap_chain(const SwapChainCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_shader(const ShaderCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_render_pass(const RenderPassCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_command_pool(const CommandPoolCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_texture_view(const TextureViewCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_semaphore(const SemaphoreCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_fence(const FenceCreateInfo& in_create_info) override;
	cb::Result<BackendDeviceResource, Result> create_pipeline_layout(const PipelineLayoutCreateInfo& in_create_info) override;
	
	void destroy_buffer(const BackendDeviceResource& in_buffer) override;
	void destroy_swap_chain(const BackendDeviceResource& in_swap_chain) override;
	void destroy_shader(const BackendDeviceResource& in_shader) override;
	void destroy_pipeline(const BackendDeviceResource& in_pipeline) override;
	void destroy_render_pass(const BackendDeviceResource& in_render_pass) override;
	void destroy_command_pool(const BackendDeviceResource& in_command_pool) override;
	void destroy_texture_view(const BackendDeviceResource& in_texture_view) override;
	void destroy_semaphore(const BackendDeviceResource& in_semaphore) override;
	void destroy_fence(const BackendDeviceResource& in_fence) override;
	void destroy_pipeline_layout(const BackendDeviceResource& in_pipeline_layout) override;
	
	cb::Result<void*, Result> map_buffer(const BackendDeviceResource& in_buffer) override;
	void unmap_buffer(const BackendDeviceResource& in_buffer) override;

	cb::Result<std::vector<BackendDeviceResource>, Result> allocate_command_lists(const BackendDeviceResource& in_pool, 
		const uint32_t in_count) override;
	void free_command_lists(const BackendDeviceResource& in_pool, const std::vector<BackendDeviceResource>& in_lists) override;
	void reset_command_pool(const BackendDeviceResource& in_pool) override;
	
	void begin_cmd_list(const BackendDeviceResource& in_list) override;
	void cmd_begin_render_pass(const BackendDeviceResource& in_list,
		const BackendDeviceResource& in_render_pass,
		const Framebuffer& in_framebuffer,
		Rect2D in_render_area,
		std::span<ClearValue> in_clear_values) override;
	void cmd_bind_pipeline(const BackendDeviceResource& in_list, 
		const PipelineBindPoint in_bind_point, 
		const BackendDeviceResource& in_pipeline) override;
	void cmd_draw(const BackendDeviceResource& in_list,
		const uint32_t in_vertex_count,
		const uint32_t in_instance_count,
		const uint32_t in_first_vertex,
		const uint32_t in_first_instance) override;
	void cmd_end_render_pass(const BackendDeviceResource& in_list) override;
	void cmd_set_viewports(const BackendDeviceResource& in_list, const uint32_t in_first_viewport, const std::span<Viewport>& in_viewports) override;
	void cmd_set_scissors(const BackendDeviceResource& in_list, const uint32_t in_first_scissor, const std::span<Rect2D>& in_scissors) override;
	void end_cmd_list(const BackendDeviceResource& in_list) override;

	Result acquire_swapchain_image(const BackendDeviceResource& in_swapchain, 
		const BackendDeviceResource& in_signal_semaphore) override;
	void present(const BackendDeviceResource& in_swapchain,
		const std::span<BackendDeviceResource>& in_wait_semaphores) override;
	BackendDeviceResource get_swapchain_backbuffer_view(const BackendDeviceResource& in_device) override;

	Result wait_for_fences(const std::span<BackendDeviceResource>& in_fences, 
		const bool in_wait_for_all, 
		const uint64_t in_timeout) override;

	void reset_fences(const std::span<BackendDeviceResource>& in_fences) override;
	
	void queue_submit(const QueueType& in_type,
		const std::span<BackendDeviceResource>& in_command_lists,
		const std::span<BackendDeviceResource>& in_wait_semaphores = {},
		const std::span<PipelineStageFlags>& in_wait_pipeline_stages = {},
		const std::span<BackendDeviceResource>& in_signal_semaphores = {},
		const BackendDeviceResource& in_fence = null_backend_resource) override;
	
	[[nodiscard]] VulkanBackend& get_backend() const { return backend; }
	[[nodiscard]] VkDevice get_device() const { return device_wrapper.device.device; }
	[[nodiscard]] VkPhysicalDevice get_physical_device() const { return device_wrapper.device.physical_device.physical_device; }
	[[nodiscard]] VmaAllocator get_allocator() const { return allocator; }
	[[nodiscard]] VkQueue get_present_queue() { return device_wrapper.device.get_queue(vkb::QueueType::graphics).value(); }
private:
	VulkanBackend& backend;
	VmaAllocator allocator;
	DeviceWrapper device_wrapper;
	SurfaceManager surface_manager;
	FramebufferManager framebuffer_manager;
};
	
}