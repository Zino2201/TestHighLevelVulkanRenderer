#include "engine/gfx/Device.hpp"
#include "engine/gfx/BackendDevice.hpp"

namespace cb::gfx
{

using namespace detail;

Device* current_device = nullptr;

Device::Device(Backend& in_backend, 
	std::unique_ptr<BackendDevice>&& in_backend_device) : backend(in_backend),
	backend_device(std::move(in_backend_device)),
	current_frame(0)
{
	current_device = this;	
}

Device::~Device()
{
	current_device = nullptr;	
}

void Device::Frame::free_resources()
{
	
}

void Device::new_frame()
{
	backend_device->new_frame();

	static bool first_frame = true;

	if(!first_frame)
	{
		current_frame = (current_frame + 1) % max_frames_in_flight;

		/** Wait for fences before doing anything */
		if(!get_current_frame().wait_fences.empty())
		{
			wait_for_fences(get_current_frame().wait_fences);
			reset_fences(get_current_frame().wait_fences);
			get_current_frame().wait_fences.clear();
		}

		/** Free all resources marked to be destroyed */
		get_current_frame().free_resources();

		/** Reset commands pools */
		get_current_frame().gfx_command_pool.reset();
		get_current_frame().compute_command_pool.reset();
	}
	else
	{
		first_frame = true;
	}
}

cb::Result<ShaderHandle, Result> Device::create_shader(const ShaderCreateInfo& in_create_info)
{
	auto result = backend_device->create_shader(in_create_info);
	if(!result)
		return result.get_error();

	auto shader = shaders.allocate(*this, result.get_value());
	return make_result(cast_resource_ptr<ShaderHandle>(shader));
}

cb::Result<SwapchainHandle, Result> Device::create_swapchain(const SwapChainCreateInfo& in_create_info)
{
	auto result = backend_device->create_swap_chain(in_create_info);
	if(!result)
		return result.get_error();

	auto swapchain = swapchains.allocate(*this, result.get_value());
	return make_result(cast_resource_ptr<SwapchainHandle>(swapchain));
}

cb::Result<PipelineLayoutHandle, Result> Device::create_pipeline_layout(const PipelineLayoutCreateInfo& in_create_info)
{
	auto result = backend_device->create_pipeline_layout(in_create_info);
	if(!result)
		return result.get_error();

	auto layout = pipeline_layouts.allocate(*this, result.get_value());
	return make_result(cast_resource_ptr<PipelineLayoutHandle>(layout));
}

cb::Result<PipelineHandle, Result> Device::create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info)
{
	auto result = backend_device->create_gfx_pipeline(in_create_info);
	if(!result)
		return result.get_error();

	auto pipeline = pipelines.allocate(*this, result.get_value());
	return make_result(cast_resource_ptr<PipelineHandle>(pipeline));
}

void Device::destroy_shader(const ShaderHandle& in_shader)
{
	shaders.free(cast_handle<Shader>(in_shader));	
}

void Device::destroy_swapchain(const SwapchainHandle& in_swapchain)
{
	swapchains.free(cast_handle<Swapchain>(in_swapchain));	
}

void Device::destroy_pipeline(const PipelineHandle& in_pipeline)
{
	pipelines.free(cast_handle<Pipeline>(in_pipeline));		
}

void Device::destroy_pipeline_layout(const PipelineLayoutHandle& in_pipeline_layout)
{
	pipeline_layouts.free(cast_handle<PipelineLayout>(in_pipeline_layout));		
}

CommandListHandle Device::allocate_cmd_list(const QueueType& in_type)
{
	
}

void Device::wait_for_fences(const std::span<FenceHandle>& in_fences, const bool in_wait_for_all, const uint64_t in_timeout)
{
	std::vector<BackendDeviceResource> fences;
	fences.reserve(in_fences.size());
	for(const auto& fence : in_fences)
		fences.emplace_back(cast_handle<Fence>(fence)->get_resource());
	
	backend_device->wait_for_fences(fences, in_wait_for_all, in_timeout);
}

void Device::reset_fences(const std::span<FenceHandle>& in_fences)
{
	std::vector<BackendDeviceResource> fences;
	fences.reserve(in_fences.size());
	for(const auto& fence : in_fences)
		fences.emplace_back(cast_handle<Fence>(fence)->get_resource());
	
	backend_device->reset_fences(fences);
}

Device* get_device()
{
	return current_device;
}

}