#include "engine/gfx/Device.hpp"
#include "engine/gfx/BackendDevice.hpp"

namespace cb::gfx
{

using namespace detail;

Device* current_device = nullptr;

/** Resources dtor */
Buffer::~Buffer()
{
	device.get_backend_device()->destroy_buffer(resource);	
}

Texture::~Texture()
{
	if(is_texture_from_swapchain())
		device.get_backend_device()->destroy_texture(resource);
}

TextureView::~TextureView()
{
	if(texture.is_texture_from_swapchain())
		device.get_backend_device()->destroy_texture_view(resource);
}

Swapchain::Swapchain(Device& in_device,
	const SwapChainCreateInfo& in_create_info,
	const BackendDeviceResource& in_swapchain) : BackendResourceWrapper(in_device, in_swapchain),
	current_image(0)
{
	auto bck_textures = device.get_backend_device()->get_swapchain_backbuffers(in_swapchain);
	auto format = device.get_backend_device()->get_swapchain_format(in_swapchain);
	for(const auto& texture : bck_textures)
	{
		textures.emplace_back(Device::cast_resource_ptr<TextureHandle>(in_device.textures.allocate(device,
			TextureCreateInfo(
				TextureType::Tex2D,
				MemoryUsage::GpuToCpu,
				format,
				in_create_info.width,
				in_create_info.height,
				1,
				1,
				1,
				SampleCountFlagBits::Count1,
				TextureUsageFlags(TextureUsageFlagBits::ColorAttachment)),
			true,
			texture)));
	}
	
	auto bck_views = device.get_backend_device()->get_swapchain_backbuffer_views(in_swapchain);
	size_t i = 0;
	for(const auto& view : bck_views)
	{
		views.emplace_back(Device::cast_resource_ptr<TextureViewHandle>(in_device.texture_views.allocate(device,
			*Device::cast_handle<Texture>(textures[i]),
			TextureViewCreateInfo(
				bck_textures[i],
				TextureViewType::Tex2D,
				format,
				TextureSubresourceRange(TextureAspectFlags(TextureAspectFlagBits::Color), 0, 1, 0, 1)
			),
			view,
			false)));
		i++;
	}
}

Swapchain::~Swapchain()
{
	for(const auto& texture : textures)
		device.textures.free(Device::cast_handle<Texture>(texture));

	for(const auto& view : views)
		device.texture_views.free(Device::cast_handle<TextureView>(view));
	
	device.get_backend_device()->destroy_swap_chain(resource);
}

Fence::~Fence()
{
	device.get_backend_device()->destroy_fence(resource);
}

Semaphore::~Semaphore()
{
	device.get_backend_device()->destroy_semaphore(resource);
}

PipelineLayout::~PipelineLayout()
{
	device.get_backend_device()->destroy_pipeline_layout(resource);
}

Pipeline::~Pipeline()
{
	device.get_backend_device()->destroy_pipeline(resource);
}

Shader::~Shader()
{
	device.get_backend_device()->destroy_shader(resource);
}

/** Device */

Device::Device(Backend& in_backend, 
	std::unique_ptr<BackendDevice>&& in_backend_device) : backend(in_backend),
	backend_device(std::move(in_backend_device)),
	current_frame(0)
{
	current_device = this;

	frames.resize(Device::max_frames_in_flight);
}

Device::~Device()
{
	current_device = nullptr;	
}

Device::Frame::Frame() : gfx_command_pool(QueueType::Gfx),
	compute_command_pool(QueueType::Compute)
{
	auto fence = get_device()->create_fence(FenceCreateInfo());
	gfx_fence = fence.get_value();
	max flight = 2, destroy/unique
}

void Device::Frame::free_resources()
{
	for(auto& buffer : expired_buffers)
		get_device()->destroy_buffer(buffer);

	for(auto& texture : expired_textures)
		get_device()->destroy_texture(texture);

	for(auto& texture_view : expired_texture_views)
		get_device()->destroy_texture_view(texture_view);

	for(auto& shader : expired_shaders)
		get_device()->destroy_shader(shader);

	for(auto& pipeline_layout : expired_pipeline_layouts)
		get_device()->destroy_pipeline_layout(pipeline_layout);

	for(auto& pipeline : expired_pipelines)
		get_device()->destroy_pipeline(pipeline);
	
	for(auto& fence : expired_fences)
		get_device()->destroy_fence(fence);
	
	for(auto& semaphore : expired_semaphores)
		get_device()->destroy_semaphore(semaphore);
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
		}

		/** Free all resources marked to be destroyed */
		get_current_frame().free_resources();
		get_current_frame().reset();
	}
	else
	{
		first_frame = false;
	}
}

void Device::end_frame()
{
	/** Submit to queues */
	submit_queue(QueueType::Gfx);
}

void Device::submit(CommandListHandle in_cmd_list,
	const std::span<SemaphoreHandle>& in_wait_semaphores,
	const std::span<SemaphoreHandle>& in_signal_semaphores)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	backend_device->end_cmd_list(list->get_resource());
	
	switch(list->get_queue_type())
	{
	case QueueType::Gfx:
		get_current_frame().gfx_lists.emplace_back(in_cmd_list);
		for(const auto& semaphore : in_wait_semaphores)
			get_current_frame().gfx_wait_semaphores.emplace_back(semaphore);

		for(const auto& semaphore : in_signal_semaphores)
			get_current_frame().gfx_signal_semaphores.emplace_back(semaphore);
		break;
	default:
		break;
	}
}

void Device::submit_queue(const QueueType& in_type)
{
	std::vector<CommandListHandle>* lists = nullptr;
	FenceHandle fence;
	std::vector<SemaphoreHandle>* wait_semaphores_handles = nullptr;
	std::vector<SemaphoreHandle>* signal_semaphores_handles = nullptr;
	
	switch(in_type)
	{
	case QueueType::Gfx:
		lists = &get_current_frame().gfx_lists;
		fence = get_current_frame().gfx_fence;
		wait_semaphores_handles = &get_current_frame().gfx_wait_semaphores;
		signal_semaphores_handles = &get_current_frame().gfx_signal_semaphores;
		break;
	default:
		break;
	}

	CB_CHECK(fence && wait_semaphores_handles && signal_semaphores_handles);

	std::vector<BackendDeviceResource> wait_semaphores;
	std::vector<PipelineStageFlags> wait_pipeline_flags;
	wait_semaphores.reserve(wait_semaphores_handles->size());
	wait_pipeline_flags.reserve(wait_semaphores_handles->size());
	for(const auto& handle : *wait_semaphores_handles)
	{
		wait_semaphores.emplace_back(cast_handle<Semaphore>(handle)->get_resource());
		wait_pipeline_flags.emplace_back(PipelineStageFlags(PipelineStageFlagBits::TopOfPipe));
	}
	
	std::vector<BackendDeviceResource> signal_semaphores;
	signal_semaphores.reserve(signal_semaphores_handles->size());
	for(const auto& handle : *signal_semaphores_handles)
		signal_semaphores.emplace_back(cast_handle<Semaphore>(handle)->get_resource());

	get_current_frame().wait_fences.emplace_back(fence);
	
	if(lists && !lists->empty())
	{
		std::vector<BackendDeviceResource> cmds;
		cmds.reserve(lists->size());
		for(const auto& list : *lists)
			cmds.emplace_back(cast_handle<CommandList>(list)->get_resource());
		
		get_backend_device()->queue_submit(in_type,
			cmds,
			wait_semaphores,
			wait_pipeline_flags,
			signal_semaphores,
			cast_handle<Fence>(fence)->get_resource());
	}
}

cb::Result<BufferHandle, Result> Device::create_buffer(const BufferInfo& in_create_info)
{
	auto result = backend_device->create_buffer(in_create_info.info);
	if(!result)
		return result.get_error();

	auto buffer = buffers.allocate(*this, result.get_value());
	auto handle = cast_resource_ptr<BufferHandle>(buffer);
	
	if(!in_create_info.initial_data.empty())
	{
		if(in_create_info.info.mem_usage != MemoryUsage::CpuOnly)
		{
			auto staging = create_buffer(BufferInfo::make_staging(in_create_info.info.size,
				in_create_info.initial_data));
			if(!staging)
				return result.get_error();

			auto list = allocate_cmd_list(QueueType::Gfx);

			std::array regions = { BufferCopyRegion(0, 0, in_create_info.info.size) };
			cmd_copy_buffer(list, handle, staging.get_value(), regions);
			destroy_buffer(staging.get_value());
			submit(list);
		}
		else
		{
			auto map = backend_device->map_buffer(result.get_value());
			if(!map)
				return make_error(map.get_error());
			memcpy(map.get_value(), in_create_info.initial_data.data(), in_create_info.initial_data.size());
			backend_device->unmap_buffer(result.get_value());
		}
	}
	
	return make_result(handle);
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

	auto swapchain = swapchains.allocate(*this, in_create_info, result.get_value());
	return make_result(cast_resource_ptr<SwapchainHandle>(swapchain));
}

cb::Result<FenceHandle, Result> Device::create_fence(const FenceCreateInfo& in_create_info)
{
	auto result = backend_device->create_fence(in_create_info);
	if(!result)
		return result.get_error();

	auto fence = fences.allocate(*this, result.get_value());
	return make_result(cast_resource_ptr<FenceHandle>(fence));
}

cb::Result<SemaphoreHandle, Result> Device::create_semaphore(const SemaphoreCreateInfo& in_create_info)
{
	auto result = backend_device->create_semaphore(in_create_info);
	if(!result)
		return result.get_error();

	auto semaphore = semaphores.allocate(*this, result.get_value());
	return make_result(cast_resource_ptr<SemaphoreHandle>(semaphore));
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

void Device::destroy_buffer(const BufferHandle& in_buffer)
{
	buffers.free(cast_handle<Buffer>(in_buffer));
}

void Device::destroy_texture(const TextureHandle& in_texture)
{
	textures.free(cast_handle<Texture>(in_texture));
}

void Device::destroy_texture_view(const TextureViewHandle& in_texture_view)
{
	texture_views.free(cast_handle<TextureView>(in_texture_view));
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

void Device::destroy_fence(const FenceHandle& in_fence)
{
	fences.free(cast_handle<Fence>(in_fence));			
}

void Device::destroy_semaphore(const SemaphoreHandle& in_semaphore)
{
	semaphores.free(cast_handle<Semaphore>(in_semaphore));			
}

CommandListHandle Device::allocate_cmd_list(const QueueType& in_type)
{
	CommandListHandle list;
	
	switch(in_type)
	{
	case QueueType::Gfx:
		list = get_current_frame().gfx_command_pool.allocate_cmd_list();
		break;
	default:
		CB_ASSERT(false);
		break;
	}

	CB_CHECK(list);
	backend_device->begin_cmd_list(cast_handle<CommandList>(list)->get_resource());
	return list;
}

void Device::wait_for_fences(const std::span<FenceHandle>& in_fences, const bool in_wait_for_all, const uint64_t in_timeout)
{
	std::vector<BackendDeviceResource> wait_fences;
	wait_fences.reserve(in_fences.size());
	for(const auto& fence : in_fences)
		wait_fences.emplace_back(cast_handle<Fence>(fence)->get_resource());
	
	backend_device->wait_for_fences(wait_fences, in_wait_for_all, in_timeout);
}

void Device::reset_fences(const std::span<FenceHandle>& in_fences)
{
	std::vector<BackendDeviceResource> wait_fences;
	wait_fences.reserve(in_fences.size());
	for(const auto& fence : in_fences)
		wait_fences.emplace_back(cast_handle<Fence>(fence)->get_resource());
	
	backend_device->reset_fences(wait_fences);
}

/** Commands */

void Device::cmd_begin_render_pass(const CommandListHandle& in_cmd_list,
	const RenderPassInfo& in_info)
{
	CB_CHECK(in_info.render_area.width > 0 && in_info.render_area.height > 0);

	std::vector<AttachmentDescription> attachment_descriptions;
	std::vector<BackendDeviceResource> attachments;
	attachments.reserve(in_info.color_attachments.size() + 1);
	attachment_descriptions.reserve(in_info.color_attachments.size() + 1);

	Framebuffer framebuffer;
	framebuffer.width = in_info.render_area.width;
	framebuffer.height = in_info.render_area.height;
	framebuffer.layers = 1;
	
	for(size_t i = 0; i < in_info.color_attachments.size(); ++i)
	{
		auto view = cast_handle<TextureView>(in_info.color_attachments[i]);
		
		AttachmentDescription desc(view->get_create_info().format,
			view->get_texture().get_create_info().sample_count,
			AttachmentLoadOp::DontCare,
			AttachmentStoreOp::DontCare,
			AttachmentLoadOp::DontCare,
			AttachmentStoreOp::DontCare,
			TextureLayout::Undefined,
			TextureLayout::ColorAttachment);

		if(in_info.clear_attachment_flags & (1 << i))
			desc.load_op = AttachmentLoadOp::Clear;
		
		if(in_info.load_attachment_flags & (1 << i))
			desc.load_op = AttachmentLoadOp::Load;

		if(in_info.store_attachment_flags & (1 << i))
			desc.store_op = AttachmentStoreOp::Store;

		if(view->get_texture().is_texture_from_swapchain())
			desc.final_layout = TextureLayout::Present;

		attachments.push_back(view->get_resource());
		attachment_descriptions.push_back(desc);
	}

	if(in_info.depth_stencil_attachment)
	{
		auto view = cast_handle<TextureView>(in_info.depth_stencil_attachment);
		attachment_descriptions.emplace_back(view->get_create_info().format,
			view->get_texture().get_create_info().sample_count,
			AttachmentLoadOp::Clear,
			AttachmentStoreOp::Store,
			AttachmentLoadOp::Clear,
			AttachmentStoreOp::Store,
			TextureLayout::Undefined,
			TextureLayout::DepthStencilAttachment);
	}

	std::vector<SubpassDescription> subpasses;
	subpasses.reserve(in_info.subpasses.size());
	for(const auto& subpass : in_info.subpasses)
	{
		auto process_attachments = [&](const std::span<uint32_t>& in_indices, 
			const TextureLayout in_layout) -> std::vector<AttachmentReference>
		{
			std::vector<AttachmentReference> refs;
			refs.reserve(in_indices.size());

			for(const auto& index : in_indices)
			{
				refs.emplace_back(AttachmentReference(index,
					in_layout));
			}

			return refs;
		};

		std::vector<AttachmentReference> input_attachments = process_attachments(subpass.input_attachments, 
			TextureLayout::ShaderReadOnly);

		std::vector<AttachmentReference> color_attachments = process_attachments(subpass.color_attachments,
			TextureLayout::ColorAttachment);

		std::vector<AttachmentReference> resolve_attachments = process_attachments(subpass.resolve_attachments,
			TextureLayout::ColorAttachment);

		/** Depth/stencil attachment always at the end ! */
		if(in_info.depth_stencil_attachment)
		{
			AttachmentReference depth_attachment(static_cast<uint32_t>(attachment_descriptions.size()) - 1,
				subpass.mode == RenderPassInfo::DepthStencilMode::ReadWrite 
					? TextureLayout::DepthStencilAttachment : TextureLayout::DepthReadOnly);
		}
		
		subpasses.push_back(SubpassDescription(input_attachments,
			color_attachments,
			resolve_attachments,
			{},
			{}));
	}
	
	auto render_pass = get_or_create_render_pass(RenderPassCreateInfo(attachment_descriptions, subpasses));
	auto list = cast_handle<CommandList>(in_cmd_list);

	list->render_pass = render_pass;

	framebuffer.attachments = attachments;
	
	backend_device->cmd_begin_render_pass(
		list->get_resource(),
		render_pass,
		framebuffer,
		in_info.render_area,
		in_info.clear_values);

	std::array viewports = { Viewport(0, 0, 
		static_cast<float>(framebuffer.width), static_cast<float>(framebuffer.height) )};
	std::array scissors = { Rect2D(0, 0, framebuffer.width, framebuffer.height )};
	backend_device->cmd_set_viewports(list->get_resource(), 0, viewports);
	backend_device->cmd_set_scissors(list->get_resource(), 0, scissors);
}

void Device::cmd_draw(const CommandListHandle& in_cmd_list,
	const uint32_t in_vertex_count, 
	const uint32_t in_instance_count, 
	const uint32_t in_first_vertex, 
	const uint32_t in_first_index)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	if(list->pipeline_state_dirty)
		update_pipeline_state(list);
	
	backend_device->cmd_draw(list->get_resource(),
		in_vertex_count,
		in_instance_count,
		in_first_vertex,
		in_first_index);	
}

void Device::cmd_set_render_pass_state(const CommandListHandle& in_cmd_list, 
	const PipelineRenderPassState& in_state)
{
	auto list = cast_handle<CommandList>(in_cmd_list);

	list->render_pass_state = in_state;	
	list->pipeline_state_dirty = true;
}

void Device::cmd_set_material_state(const CommandListHandle& in_cmd_list, 
	const PipelineMaterialState& in_state)
{
	auto list = cast_handle<CommandList>(in_cmd_list);

	list->material_state = in_state;
	list->pipeline_state_dirty = true;
}

void Device::cmd_bind_pipeline_layout(const CommandListHandle& in_cmd_list, 
	const PipelineLayoutHandle& in_handle)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	list->pipeline_layout = in_handle;
}

void Device::cmd_end_render_pass(const CommandListHandle& in_cmd_list)
{
	backend_device->cmd_end_render_pass(cast_handle<CommandList>(in_cmd_list)->get_resource());
}

void Device::cmd_copy_buffer(const CommandListHandle& in_cmd_list, 
	const BufferHandle& in_src_buffer, 
	const BufferHandle& in_dst_buffer, 
	const std::span<BufferCopyRegion>& in_regions)
{
	CB_CHECK(!in_regions.empty());

	auto list = cast_handle<CommandList>(in_cmd_list);
	auto src = cast_handle<Buffer>(in_src_buffer);
	auto dst = cast_handle<Buffer>(in_dst_buffer);
	
	backend_device->cmd_copy_buffer(list->get_resource(),
		src->get_resource(),
		dst->get_resource(),
		in_regions);	
}

Result Device::acquire_swapchain_texture(const SwapchainHandle& in_swapchain,
	const SemaphoreHandle& in_signal_semaphore)
{
	auto swapchain = cast_handle<Swapchain>(in_swapchain);
	auto semaphore = cast_handle<Semaphore>(in_signal_semaphore);
	
	auto pair = backend_device->acquire_swapchain_image(swapchain->get_resource(),
		semaphore ? semaphore->get_resource() : null_backend_resource);
	swapchain->current_image = pair.second;
	return pair.first;
}

void Device::present(const SwapchainHandle& in_swapchain, 
	const std::span<SemaphoreHandle>& in_wait_semaphores)
{
	auto swapchain = cast_handle<Swapchain>(in_swapchain);
	std::vector<BackendDeviceResource> wait_semaphores;
	wait_semaphores.reserve(in_wait_semaphores.size());

	for(const auto& semaphore : in_wait_semaphores)
		wait_semaphores.emplace_back(cast_handle<Semaphore>(semaphore)->get_resource());

	backend_device->present(swapchain->get_resource(), wait_semaphores);
}

TextureViewHandle Device::get_swapchain_backbuffer_view(const SwapchainHandle& in_swapchain) const
{
	return cast_handle<Swapchain>(in_swapchain)->get_backbuffer_view();
}

void Device::update_pipeline_state(CommandList* in_cmd_list)
{
	CB_CHECKF(in_cmd_list->pipeline_layout, "No pipeline layout was bound!");
	
	GfxPipelineCreateInfo create_info(in_cmd_list->material_state.stages,
		in_cmd_list->material_state.vertex_input,
		in_cmd_list->material_state.input_assembly,
		in_cmd_list->material_state.rasterizer,
		in_cmd_list->render_pass_state.multisampling,
		in_cmd_list->render_pass_state.depth_stencil,
		in_cmd_list->render_pass_state.color_blend,
		cast_handle<PipelineLayout>(in_cmd_list->pipeline_layout)->get_resource(),
		in_cmd_list->render_pass,
		0);
	auto pipeline = get_or_create_pipeline(create_info);
	
	backend_device->cmd_bind_pipeline(
		in_cmd_list->get_resource(),
		PipelineBindPoint::Gfx,
		pipeline);
	
	in_cmd_list->pipeline_state_dirty = false;	
}

BackendDeviceResource Device::get_or_create_render_pass(const RenderPassCreateInfo& in_create_info)
{
	auto it = render_passes.find(in_create_info);
	if(it != render_passes.end())
		return it->second;

	auto rp = backend_device->create_render_pass(in_create_info);
	CB_ASSERT(rp.has_value());
	render_passes.insert({ in_create_info, rp.get_value() });
	return rp.get_value();
}

BackendDeviceResource Device::get_or_create_pipeline(const GfxPipelineCreateInfo& in_create_info)
{
	auto it = gfx_pipelines.find(in_create_info);
	if(it != gfx_pipelines.end())
		return it->second;

	auto pipeline = backend_device->create_gfx_pipeline(in_create_info);
	CB_ASSERT(pipeline.has_value());
	gfx_pipelines.insert({ in_create_info, pipeline.get_value() });
	return pipeline.get_value();
}

Device* get_device()
{
	return current_device;
}

}