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
#include "PipelineLayout.hpp"
#include <thread>
#include <robin_hood.h>

namespace cb::gfx
{

class Backend;
class BackendDevice;
class Device;

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
private:
	Device& device;
	BackendDeviceResource resource;	
};

class Shader : public BackendResourceWrapper
{
public:
	Shader(Device& in_device,
		const BackendDeviceResource& in_shader) : BackendResourceWrapper(in_device, in_shader) {}
};

class Swapchain : public BackendResourceWrapper
{
public:
	Swapchain(Device& in_device,
		const BackendDeviceResource& in_swapchain) : BackendResourceWrapper(in_device, in_swapchain) {}
};

class CommandPool : public BackendResourceWrapper
{
public:
	CommandPool(Device& in_device,
		const BackendDeviceResource& in_command_pool) : BackendResourceWrapper(in_device, in_command_pool) {}
};

class Pipeline : public BackendResourceWrapper
{
public:
	Pipeline(Device& in_device,
		const BackendDeviceResource& in_pipeline) : BackendResourceWrapper(in_device, in_pipeline) {}
};

class PipelineLayout : public BackendResourceWrapper
{
public:
	PipelineLayout(Device& in_device,
		const BackendDeviceResource& in_pipeline_layout) : BackendResourceWrapper(in_device, in_pipeline_layout) {}
};

class CommandList : public BackendResourceWrapper
{
public:
	CommandList(Device& in_device,
		const BackendDeviceResource& in_list) : BackendResourceWrapper(in_device, in_list) {}
};

class Fence : public BackendResourceWrapper
{
public:
	Fence(Device& in_device,
		const BackendDeviceResource& in_fence) : BackendResourceWrapper(in_device, in_fence) {}
};

template<> struct IsHandleCompatibleWith<Swapchain, SwapchainHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Shader, ShaderHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<CommandList, CommandListHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Pipeline, PipelineHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<PipelineLayout, PipelineLayoutHandle> : std::true_type {};
template<> struct IsHandleCompatibleWith<Fence, FenceHandle> : std::true_type {};

/*
 * Spawn one command pool per thread
 */
class ThreadedCommandPool
{
public:
	struct Pool
	{
		BackendDeviceResource handle;
		ThreadSafeSimplePool<CommandList> command_lists;

		Pool(const QueueType in_type);
		~Pool();

		void reset();
		CommandListHandle allocate_cmd_list();
	};
	
	ThreadedCommandPool(const QueueType in_type) : type(in_type) {}

	void reset();
private:
	Pool& get_pool()
	{
		auto it = pools.find(std::this_thread::get_id());
		if(it != pools.end())
			return it->second;

		auto it_insert = pools.insert({ std::this_thread::get_id(), Pool(type) });
		return it->first->second;
	}
private:
	QueueType type;
	robin_hood::unordered_map<std::thread::id, Pool> pools;
};

}

/**
 * A GPU device, used to communicate with it
 * The engine currently only supports one active GPU (as using multiple GPU is hard to manage)
 */
class Device final
{	
	struct Frame
	{
		detail::ThreadedCommandPool gfx_command_pool;
		detail::ThreadedCommandPool compute_command_pool;

		FenceHandle gfx_fence;
		
		std::vector<FenceHandle> wait_fences;

		/** Expired resources */
		std::vector<SwapchainHandle> expired_swapchains;
		std::vector<ShaderHandle> expired_shaders;
		std::vector<PipelineLayoutHandle> expired_pipeline_layouts;
		std::vector<PipelineHandle> expired_pipelines;

		Frame() : gfx_command_pool(QueueType::Gfx),
			compute_command_pool(QueueType::Compute) {}

		void free_resources();
	};
public:
	static constexpr size_t max_frames_in_flight = 2;

	Device(Backend& in_backend, std::unique_ptr<BackendDevice>&& in_backend_device);
	~Device();
	
	Device(const Device&) = delete;	 
	void operator=(const Device&) = delete;

	void new_frame();
	void end_frame();
	
	[[nodiscard]] cb::Result<SwapchainHandle, Result> create_swapchain(const SwapChainCreateInfo& in_create_info);
	[[nodiscard]] cb::Result<ShaderHandle, Result> create_shader(const ShaderCreateInfo& in_create_info);
	[[nodiscard]] cb::Result<PipelineLayoutHandle, Result> create_pipeline_layout(const PipelineLayoutCreateInfo& in_create_info);
	[[nodiscard]] cb::Result<PipelineHandle, Result> create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info);

	void destroy_swapchain(const SwapchainHandle& in_swapchain);
	void destroy_shader(const ShaderHandle& in_shader);
	void destroy_pipeline_layout(const PipelineLayoutHandle& in_pipeline_layout);
	void destroy_pipeline(const PipelineHandle& in_pipeline);

	[[nodiscard]] CommandListHandle allocate_cmd_list(const QueueType& in_type);

	void wait_for_fences(const std::span<FenceHandle>& in_fences, 
		const bool in_wait_for_all = true, 
		const uint64_t in_timeout);
	void reset_fences(const std::span<FenceHandle>& in_fences);
	
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

	[[nodiscard]] BackendDevice* get_backend_device() const { return backend_device.get(); }
private:
	[[nodiscard]] Frame& get_current_frame() { return frames[current_frame]; }
private:
	Backend& backend;
	std::unique_ptr<BackendDevice> backend_device;
	size_t current_frame;
	std::vector<Frame> frames;
	
	/** Resources pools */
	ThreadSafeSimplePool<detail::Shader> shaders;
	ThreadSafeSimplePool<detail::Swapchain> swapchains;
	ThreadSafeSimplePool<detail::Pipeline> pipelines;
	ThreadSafeSimplePool<detail::PipelineLayout> pipeline_layouts;
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
	UniqueDeviceResource(HandleType&& in_handle) : handle(in_handle) {}
	~UniqueDeviceResource() { if(handle) deleter()(handle); }

	UniqueDeviceResource(const UniqueDeviceResource&) = delete;
	void operator=(const UniqueDeviceResource&) = delete;

	HandleType get() const
	{
		return handle;
	}

	HandleType operator*() const
	{
		return handle;
	}
private:
	HandleType handle;
	[[no_unique_address]] Deleter deleter;
};

/**
 * Deleters
 */
struct SwapchainDeleter
{
	void operator()(const SwapchainHandle& in_handle) const
	{
		get_device()->destroy_swapchain(in_handle);
	}
};
	
}

using UniqueSwapchain = detail::UniqueDeviceResource<DeviceResourceType::SwapChain, detail::SwapchainDeleter>;
	
}