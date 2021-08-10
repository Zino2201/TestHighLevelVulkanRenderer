#pragma once

#include "engine/Core.hpp"
#include "DeviceResource.hpp"
#include "SwapChain.hpp"
#include "engine/Result.hpp"
#include "Result.hpp"
#include "engine/util/SimplePool.hpp"
#include <span>
#include "Shader.hpp"
#include "Command.hpp"
#include "GfxPipeline.hpp"
#include "Buffer.hpp"
#include "PipelineLayout.hpp"
#include "RenderPass.hpp"
#include "Sync.hpp"
#include "Rect.hpp"
#include <thread>
#include <robin_hood.h>

namespace cb::gfx
{

class Backend;
class BackendDevice;
class Device;

/**
 * Pipeline state associated to a render pass
 */
struct PipelineRenderPassState
{
	PipelineColorBlendStateCreateInfo color_blend;	
	PipelineDepthStencilStateCreateInfo depth_stencil;	
	PipelineMultisamplingStateCreateInfo multisampling;	
};

/**
 * Pipeline state associated to an instance
 */
struct PipelineMaterialState
{
	std::span<PipelineShaderStage> stages;
	PipelineVertexInputStateCreateInfo vertex_input;
	PipelineInputAssemblyStateCreateInfo input_assembly;
	PipelineRasterizationStateCreateInfo rasterizer;
};

namespace detail
{

template<typename T, typename Handle>
struct IsHandleCompatibleWith : std::false_type {};

class BackendResourceWrapper
{
public:
	BackendResourceWrapper(Device& in_device,
		const BackendDeviceResource& in_resource) : device(in_device),
	resource(in_resource) {}

	[[nodiscard]] BackendDeviceResource get_resource() const { return resource; }
protected:
	Device& device;
	BackendDeviceResource resource;	
};

class Buffer : public BackendResourceWrapper
{
public:
	Buffer(Device& in_device,
		const BackendDeviceResource& in_buffer) : BackendResourceWrapper(in_device, in_buffer) {}
	~Buffer();
};

class Texture : public BackendResourceWrapper
{
public:
	Texture(Device& in_device,
		const TextureCreateInfo& in_create_info,
		const bool in_is_swapchain_texture,
		const BackendDeviceResource& in_texture) : BackendResourceWrapper(in_device, in_texture),
		create_info(in_create_info), is_swapchain_texture(in_is_swapchain_texture) {}
	~Texture();

	[[nodiscard]] const TextureCreateInfo& get_create_info() { return create_info; }
	[[nodiscard]] bool is_texture_from_swapchain() const { return is_swapchain_texture; }
private:
	TextureCreateInfo create_info;
	bool is_swapchain_texture;
};

class TextureView : public BackendResourceWrapper
{
public:
	TextureView(Device& in_device,
		Texture& in_texture,
		const TextureViewCreateInfo& in_create_info,
		const BackendDeviceResource& in_texture_view,
		const bool in_own_resource = true) : BackendResourceWrapper(in_device, in_texture_view), texture(in_texture),
		create_info(in_create_info), own_resource(in_own_resource) {}
	~TextureView();

	[[nodiscard]] Texture& get_texture() { return texture; }
	[[nodiscard]] const TextureViewCreateInfo& get_create_info() { return create_info; }
private:
	Texture& texture;
	TextureViewCreateInfo create_info;
	bool own_resource;
};

class Shader : public BackendResourceWrapper
{
public:
	Shader(Device& in_device,
		const BackendDeviceResource& in_shader) : BackendResourceWrapper(in_device, in_shader) {}
	~Shader();
};

class Swapchain : public BackendResourceWrapper
{
	friend class Device;

public:
	Swapchain(Device& in_device,
		const SwapChainCreateInfo& in_create_info,
		const BackendDeviceResource& in_swapchain);
	~Swapchain();

	void reset_handles();

	[[nodiscard]] TextureHandle get_backbuffer() const { return textures[current_image]; }
	[[nodiscard]] TextureViewHandle get_backbuffer_view() const { return views[current_image]; }
private:
	void destroy();
private:
	SwapChainCreateInfo create_info;
	std::vector<TextureHandle> textures;
	std::vector<TextureViewHandle> views;
	uint32_t current_image;
};

class CommandPool : public BackendResourceWrapper
{
public:
	CommandPool(Device& in_device,
		const BackendDeviceResource& in_command_pool) : BackendResourceWrapper(in_device, in_command_pool) {}
	~CommandPool();
};

class PipelineLayout : public BackendResourceWrapper
{
public:
	PipelineLayout(Device& in_device,
		const BackendDeviceResource& in_pipeline_layout) : BackendResourceWrapper(in_device, in_pipeline_layout) {}
	~PipelineLayout();
};

class CommandList : public BackendResourceWrapper
{
	friend class Device;
	
public:
	CommandList(Device& in_device,
		const BackendDeviceResource& in_list,
		const QueueType& in_type) : BackendResourceWrapper(in_device, in_list),
		type(in_type), pipeline_state_dirty(false), dirty_sets_mask(0) {}

	void update_descriptors();

	[[nodiscard]] QueueType get_queue_type() const { return type; }
private:
	QueueType type;
	PipelineLayoutHandle pipeline_layout;
	BackendDeviceResource render_pass;
	PipelineRenderPassState render_pass_state;
	PipelineMaterialState material_state;
	bool pipeline_state_dirty;
	std::array<std::array<Descriptor, max_bindings>, max_descriptor_sets> descriptors;
	uint8_t dirty_sets_mask;
};

class Fence : public BackendResourceWrapper
{
public:
	Fence(Device& in_device,
		const BackendDeviceResource& in_fence) : BackendResourceWrapper(in_device, in_fence) {}
	~Fence();
};

class Semaphore : public BackendResourceWrapper
{
public:
	Semaphore(Device& in_device,
		const BackendDeviceResource& in_semaphore) : BackendResourceWrapper(in_device, in_semaphore) {}
	~Semaphore();
};

template<> struct IsHandleCompatibleWith<Buffer, BufferHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Texture, TextureHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<TextureView, TextureViewHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Swapchain, SwapchainHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Shader, ShaderHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<CommandList, CommandListHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<PipelineLayout, PipelineLayoutHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Fence, FenceHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Semaphore, SemaphoreHandle> : std::true_type {};

/*
 * Spawn one command pool per thread
 */
class ThreadedCommandPool
{
public:
	struct Pool
	{
		BackendDeviceResource handle;
		std::vector<std::unique_ptr<CommandList>> command_lists;
		QueueType type;
		size_t free_command_list;
		
		Pool() : free_command_list(0) {}
		~Pool();

		void init(const QueueType in_type);

		void reset();
		CommandListHandle allocate_cmd_list();

		Pool(const Pool&) = delete;
		void operator=(const Pool&) = delete;
	};
	
	ThreadedCommandPool(const QueueType in_type) : type(in_type) {}

	CommandListHandle allocate_cmd_list() { return get_pool().allocate_cmd_list(); }
	void reset();
private:
	Pool& get_pool()
	{
		auto it = pools.find(std::this_thread::get_id());
		if(it != pools.end())
			return it->second;

		auto& pool = pools[std::this_thread::get_id()];
		pool.init(type);
		return pool;
	}
private:
	QueueType type;
	robin_hood::unordered_map<std::thread::id, Pool> pools;
};

}

struct BufferInfo
{
	BufferCreateInfo info;
	std::span<uint8_t> initial_data;

	explicit BufferInfo(const BufferCreateInfo& in_info,
		const std::span<uint8_t>& in_initial_data = {}) : info(in_info),
		initial_data(in_initial_data) {}
	
	static BufferInfo make_staging(const size_t in_size, const std::span<uint8_t> in_initial_data = {})
	{
		return BufferInfo(BufferCreateInfo(in_size, 
			MemoryUsage::CpuOnly, 
			BufferUsageFlags()),
			in_initial_data);
	}
	
	static BufferInfo make_ubo(const size_t in_size)
	{
		return BufferInfo(BufferCreateInfo(in_size, 
			MemoryUsage::CpuToGpu, 
			BufferUsageFlags(BufferUsageFlagBits::UniformBuffer)));
	}
};

struct TextureInfo
{
	TextureCreateInfo info;

	/** Mip0 initial data */
	std::span<uint8_t> initial_data;

	explicit TextureInfo(const TextureCreateInfo& in_info,
		const std::span<uint8_t>& in_initial_data = {}) : info(in_info),
		initial_data(in_initial_data) {}

	static TextureInfo make_immutable_2d(const uint32_t in_width, 
		const uint32_t in_height,
		const Format in_format, const uint32_t in_mip_levels = 1, 
		const TextureUsageFlags in_usage_flags = TextureUsageFlags(TextureUsageFlagBits::Sampled),
		const std::span<uint8_t>& in_initial_data = {})
	{
		return TextureInfo(TextureCreateInfo(TextureType::Tex2D,
			MemoryUsage::GpuOnly,
			in_format,
			in_width,
			in_height,
			1,
			in_mip_levels,
			1,
			SampleCountFlagBits::Count1,
			in_usage_flags), in_initial_data);
	}
};

/**
 * Informations about a render pass
 */
struct RenderPassInfo
{
	enum class DepthStencilMode
	{
		ReadOnly,
		ReadWrite
	};
	
	struct Subpass
	{
		std::span<uint32_t> color_attachments;
		std::span<uint32_t> input_attachments;
		std::span<uint32_t> resolve_attachments;
		/** Depth stencil mode for the depth-stencil attachment (determine layout) */
		DepthStencilMode mode;

		Subpass(const std::span<uint32_t>& in_color_attachments,
			const std::span<uint32_t>& in_input_attachments,
			const std::span<uint32_t>& in_resolve_attachments,
			const DepthStencilMode in_mode = DepthStencilMode::ReadWrite) : color_attachments(in_color_attachments),
		input_attachments(in_input_attachments), resolve_attachments(in_resolve_attachments), mode(in_mode) {}
	};
	
	/** Attachments to use */
	std::span<TextureViewHandle> color_attachments;
	TextureViewHandle depth_stencil_attachment;

	/** Clear/load/store flags. To add attachment 2/3 to load e.g: (1 << 2) | (1 << 3) */
	uint32_t clear_attachment_flags;
	uint32_t load_attachment_flags;
	uint32_t store_attachment_flags;

	std::span<ClearValue> clear_values;


	std::span<Subpass> subpasses;

	Rect2D render_area;
};

/**
 * A GPU device, used to communicate with it
 * The engine currently only supports one active GPU (as using multiple GPU is hard to manage)
 */
class Device final
{
	friend class detail::Swapchain;
	
	struct Frame
	{
		detail::ThreadedCommandPool gfx_command_pool;
		detail::ThreadedCommandPool compute_command_pool;

		FenceHandle gfx_fence;
		std::vector<FenceHandle> wait_fences;

		/** Expired resources */
		std::vector<BufferHandle> expired_buffers;
		std::vector<TextureHandle> expired_textures;
		std::vector<TextureViewHandle> expired_texture_views;
		std::vector<SwapchainHandle> expired_swapchains;
		std::vector<ShaderHandle> expired_shaders;
		std::vector<PipelineLayoutHandle> expired_pipeline_layouts;
		std::vector<PipelineHandle> expired_pipelines;
		std::vector<FenceHandle> expired_fences;
		std::vector<SemaphoreHandle> expired_semaphores;

		std::vector<CommandListHandle> gfx_lists;
		std::vector<SemaphoreHandle> gfx_wait_semaphores;
		std::vector<SemaphoreHandle> gfx_signal_semaphores;
		bool gfx_submitted;

		Frame();

		void free_resources();
		void reset()
		{
			expired_buffers.clear();
			expired_textures.clear();
			expired_texture_views.clear();
			expired_swapchains.clear();
			expired_shaders.clear();
			expired_pipeline_layouts.clear();
			expired_pipelines.clear();

			/** Only reset commands if there are been submissions since we may have command lists created in game startup */
			//if(gfx_submitted)
				gfx_command_pool.reset();

			gfx_lists.clear();
			gfx_wait_semaphores.clear();
			gfx_signal_semaphores.clear();
			gfx_submitted = false;

			compute_command_pool.reset();

			wait_fences.clear();
		}

		void destroy()
		{
			expired_fences.emplace_back(gfx_fence);
		}
	};
public:
	static constexpr size_t max_frames_in_flight = 2;

	Device(Backend& in_backend, std::unique_ptr<BackendDevice>&& in_backend_device);
	~Device();
	
	Device(const Device&) = delete;	 
	void operator=(const Device&) = delete;
	
	void wait_idle();
	void new_frame();
	void end_frame();
	void submit(CommandListHandle in_cmd_list, 
		const std::span<SemaphoreHandle>& in_wait_semaphores = {},
		const std::span<SemaphoreHandle>& in_signal_semaphores = {});
	
	[[nodiscard]] cb::Result<BufferHandle, Result> create_buffer(BufferInfo in_create_info);
	[[nodiscard]] cb::Result<TextureHandle, Result> create_texture(TextureInfo in_create_info);
	[[nodiscard]] cb::Result<SwapchainHandle, Result> create_swapchain(const SwapChainCreateInfo& in_create_info);
	[[nodiscard]] cb::Result<SemaphoreHandle, Result> create_semaphore(const SemaphoreCreateInfo& in_create_info = {});
	[[nodiscard]] cb::Result<FenceHandle, Result> create_fence(const FenceCreateInfo& in_create_info = {});
	[[nodiscard]] cb::Result<ShaderHandle, Result> create_shader(const ShaderCreateInfo& in_create_info);
	[[nodiscard]] cb::Result<PipelineLayoutHandle, Result> create_pipeline_layout(const PipelineLayoutCreateInfo& in_create_info);

	void destroy_buffer(const BufferHandle& in_buffer);
	void destroy_texture(const TextureHandle& in_texture);
	void destroy_texture_view(const TextureViewHandle& in_texture_view);
	void destroy_swapchain(const SwapchainHandle& in_swapchain);
	void destroy_shader(const ShaderHandle& in_shader);
	void destroy_pipeline_layout(const PipelineLayoutHandle& in_pipeline_layout);
	void destroy_fence(const FenceHandle& in_fence);
	void destroy_semaphore(const SemaphoreHandle& in_semaphore);

	cb::Result<void*, Result> map_buffer(const BufferHandle& in_handle);
	void unmap_buffer(const BufferHandle& in_handle);

	[[nodiscard]] CommandListHandle allocate_cmd_list(const QueueType& in_type);

	void wait_for_fences(const std::span<FenceHandle>& in_fences, 
		const bool in_wait_for_all = true, 
		const uint64_t in_timeout = std::numeric_limits<uint64_t>::max());
	void reset_fences(const std::span<FenceHandle>& in_fences);

	/** Commands */
	void cmd_begin_render_pass(const CommandListHandle& in_cmd_list,
		const RenderPassInfo& in_info);
	void cmd_draw(const CommandListHandle& in_cmd_list,
		const uint32_t in_vertex_count, 
		const uint32_t in_instance_count,
		const uint32_t in_first_vertex,
		const uint32_t in_first_index);
	void cmd_end_render_pass(const CommandListHandle& in_cmd_list);
	void cmd_bind_vertex_buffer(const CommandListHandle& in_cmd_list,
		const BufferHandle& in_buffer,
		const uint64_t in_offset);
	void cmd_copy_buffer(const CommandListHandle& in_cmd_list,
		const BufferHandle& in_src_buffer,
		const BufferHandle& in_dst_buffer,
		const std::span<BufferCopyRegion>& in_regions);
	void cmd_copy_buffer_to_texture(const CommandListHandle& in_cmd_list,
		const BufferHandle& in_src_buffer,
		const TextureHandle& in_dst_texture,
		const TextureLayout in_dst_layout,
		const std::span<BufferTextureCopyRegion>& in_regions);
	void cmd_texture_barrier(const CommandListHandle& in_cmd_list,
		const TextureHandle& in_texture,
		const PipelineStageFlags in_src_flags,
		const TextureLayout in_src_layout,
		const AccessFlags in_src_access_flags,
		const PipelineStageFlags in_dst_flags,
		const TextureLayout in_dst_layout,
		const AccessFlags in_dst_access_flags);

	/** Pipeline management */
	void cmd_bind_pipeline_layout(const CommandListHandle& in_cmd_list, const PipelineLayoutHandle& in_handle);
	void cmd_set_render_pass_state(const CommandListHandle& in_cmd_list, const PipelineRenderPassState& in_state);
	void cmd_set_material_state(const CommandListHandle& in_cmd_list, const PipelineMaterialState& in_state);

	/** Descriptor management */
	void cmd_bind_ubo(const CommandListHandle& in_cmd_list, const uint32_t in_set, const uint32_t in_binding, 
		const BufferHandle& in_handle);
	void cmd_bind_texture_view(const CommandListHandle& in_cmd_list, const uint32_t in_set, const uint32_t in_binding, 
		const TextureViewHandle& in_handle);

	/** Swapchain */
	Result acquire_swapchain_texture(const SwapchainHandle& in_swapchain,
		const SemaphoreHandle& in_signal_semaphore = SemaphoreHandle());
	void present(const SwapchainHandle& in_swapchain,
		const std::span<SemaphoreHandle>& in_wait_semaphores = {});
	TextureViewHandle get_swapchain_backbuffer_view(const SwapchainHandle& in_swapchain) const;
	BackendDeviceResource get_swapchain_backend_handle(const SwapchainHandle& in_swapchain) const;

	template<typename T>
	[[nodiscard]] static T* cast_handle(const auto& in_handle)
		requires detail::IsHandleCompatibleWith<T, std::decay_t<decltype(in_handle)>>::value
	{
		return reinterpret_cast<T*>(in_handle.get_handle());		
	}

	template<typename Handle>
	[[nodiscard]] static Handle cast_resource_ptr(auto* in_resource)
		requires detail::IsHandleCompatibleWith<std::remove_pointer_t<decltype(in_resource)>, Handle>::value
	{
		return Handle(reinterpret_cast<uint64_t>(in_resource));
	}

	static BackendDeviceResource get_backend_shader(const ShaderHandle& in_handle)
	{
		return cast_handle<detail::Shader>(in_handle)->get_resource();
	}

	[[nodiscard]] BackendDevice* get_backend_device() const { return backend_device.get(); }
private:
	void update_pipeline_state(detail::CommandList* in_cmd_list);
	void submit_queue(const QueueType& in_type);
	BackendDeviceResource get_or_create_render_pass(const RenderPassCreateInfo& in_create_info);
	BackendDeviceResource get_or_create_pipeline(const GfxPipelineCreateInfo& in_create_info);
	
	[[nodiscard]] Frame& get_current_frame() { return frames[current_frame]; }
private:
	Backend& backend;
	std::unique_ptr<BackendDevice> backend_device;
	size_t current_frame;
	std::vector<Frame> frames;

	robin_hood::unordered_map<RenderPassCreateInfo, BackendDeviceResource> render_passes;
	robin_hood::unordered_map<GfxPipelineCreateInfo, BackendDeviceResource> gfx_pipelines;
	
	/** Resources pools */
	ThreadSafeSimplePool<detail::Buffer> buffers;
	ThreadSafeSimplePool<detail::Texture> textures;
	ThreadSafeSimplePool<detail::TextureView> texture_views;
	ThreadSafeSimplePool<detail::Shader> shaders;
	ThreadSafeSimplePool<detail::Swapchain> swapchains;
	ThreadSafeSimplePool<detail::PipelineLayout> pipeline_layouts;
	ThreadSafeSimplePool<detail::Fence> fences;
	ThreadSafeSimplePool<detail::Semaphore> semaphores;
};
	
/**
 * Get the currently used device
 */
Device* get_device();

/**
 * Smart device resources
 */

namespace detail
{
/**
 * std::unique_ptr-like for DeviceResource
 */
template<DeviceResourceType Type, typename Deleter>
struct UniqueDeviceResource
{
	using HandleType = DeviceResource<Type>;

	UniqueDeviceResource() {}
	explicit UniqueDeviceResource(HandleType in_handle) { reset(in_handle); }
	~UniqueDeviceResource() { destroy(); }

	UniqueDeviceResource(const UniqueDeviceResource&) = delete;
	void operator=(const UniqueDeviceResource&) = delete;

	UniqueDeviceResource(UniqueDeviceResource&& other) noexcept
	{
		reset(std::exchange(other.handle, HandleType()));
	}

	UniqueDeviceResource& operator=(UniqueDeviceResource&& other) noexcept
	{
		reset(std::exchange(other.handle, HandleType()));
		return *this;
	}

	HandleType free()
	{
		HandleType freed_handle = handle;
		handle = HandleType();
		return freed_handle;
	}

	void reset(const HandleType& in_new_handle = HandleType())
	{
		destroy();
		handle = in_new_handle;
	}

	HandleType get() const
	{
		return handle;
	}

	HandleType operator*() const
	{
		return handle;
	}
private:
	void destroy()
	{
		if(handle)
		{
			Deleter()(handle);
			handle = HandleType();
		}
	}
private:
	HandleType handle;
	[[no_unique_address]] Deleter deleter;
};

/**
 * Deleters
 */
#define CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(UpperCaseName, LowerCaseName, Type) \
	namespace detail \
	{ \
	struct SmartDeleter_##UpperCaseName\
	{ \
		void operator()(Type in_handle) const \
		{ \
			get_device()->destroy_##LowerCaseName(in_handle); \
		} \
	}; \
	} \
	using Unique##UpperCaseName = detail::UniqueDeviceResource<DeviceResourceType::UpperCaseName, detail::SmartDeleter_##UpperCaseName>;

}

CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(Buffer, buffer, BufferHandle);
CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(Texture, texture, TextureHandle);
CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(TextureView, texture_view, TextureViewHandle);
CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(PipelineLayout, pipeline_layout, PipelineLayoutHandle);
CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(Shader, shader, ShaderHandle);
CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(Swapchain, swapchain, SwapchainHandle);
CB_GFX_DECLARE_SMART_DEVICE_RESOURCE(Semaphore, semaphore, SemaphoreHandle);

}