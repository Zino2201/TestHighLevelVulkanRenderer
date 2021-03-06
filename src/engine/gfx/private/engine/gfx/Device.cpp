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
	if(!is_texture_from_swapchain())
		device.get_backend_device()->destroy_texture(resource);
}

TextureView::~TextureView()
{
	/** texture maybe a dangling ref, don't access it ! */
	if(!is_view_from_swapchain)
		device.get_backend_device()->destroy_texture_view(resource);
}

Swapchain::Swapchain(Device& in_device,
	const SwapChainCreateInfo& in_create_info,
	const BackendDeviceResource& in_swapchain,
	const std::string_view& in_debug_name) : BackendResourceWrapper(in_device, in_swapchain, in_debug_name),
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
			texture,
			"Swapchain Texture")));
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
			"Swapchain Texture View")));
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

Shader::~Shader()
{
	device.get_backend_device()->destroy_shader(resource);
}

Sampler::~Sampler()
{
	device.get_backend_device()->destroy_sampler(resource);
}

/** Command List */

void CommandList::prepare_draw()
{
	if(pipeline_state_dirty)
		update_pipeline_state();

	update_descriptors();
}

void CommandList::update_pipeline_state()
{
	CB_CHECKF(pipeline_layout, "No pipeline layout was bound!");
	
	GfxPipelineCreateInfo create_info(material_state.stages,
		material_state.vertex_input,
		material_state.input_assembly,
		material_state.rasterizer,
		render_pass_state.multisampling,
		render_pass_state.depth_stencil,
		render_pass_state.color_blend,
		Device::cast_handle<PipelineLayout>(pipeline_layout)->get_resource(),
		render_pass,
		0);
	auto pipeline = device.get_or_create_pipeline(create_info);
	
	device.get_backend_device()->cmd_bind_pipeline(
		resource,
		PipelineBindPoint::Gfx,
		pipeline);
	
	pipeline_state_dirty = false;	
}

void CommandList::update_descriptors()
{
	if(dirty_sets_mask == 0)
		return;

	auto layout = Device::cast_handle<PipelineLayout>(pipeline_layout);

	std::vector<BackendDeviceResource> sets;
	sets.reserve(max_descriptor_sets);
	for(uint32_t i = 0; i < max_descriptor_sets; ++i)
	{
		if(dirty_sets_mask & (1 << i))
		{
			auto result = device.get_backend_device()->allocate_descriptor_set(layout->get_resource(),
				i,
				descriptors[i]);
			sets.emplace_back(result.get_value());
		}
	}

	device.get_backend_device()->cmd_bind_descriptor_sets(resource,
		layout->get_resource(),
		sets);
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
	for(auto& frame : frames)
	{
		if(!frame.wait_fences.empty())
		{
			wait_for_fences(frame.wait_fences);
		}

		frame.destroy();
		frame.free_resources();
		frame.reset();
	}

	for(auto& [create_info, pipeline] : gfx_pipelines)
		get_backend_device()->destroy_pipeline(pipeline);

	for(auto& [create_info, rp] : render_passes)
		get_backend_device()->destroy_render_pass(rp);

	CB_CHECKF(buffers.get_size() == 0, "Some buffers have not been freed before deleting the device!");
	CB_CHECKF(textures.get_size() == 0, "Some textures have not been freed before deleting the device!");
	CB_CHECKF(texture_views.get_size() == 0, "Some textures have not been freed before deleting the device!");
	CB_CHECKF(swapchains.get_size() == 0, "Some texture views have not been freed before deleting the device!");
	CB_CHECKF(pipeline_layouts.get_size() == 0, "Some pipeline layouts have not been freed before deleting the device!");
	CB_CHECKF(shaders.get_size() == 0, "Some shaders have not been freed before deleting the device!");
	CB_CHECKF(semaphores.get_size() == 0, "Some semaphores have not been freed before deleting the device!");
	CB_CHECKF(fences.get_size() == 0, "Some fences have not been freed before deleting the device!");
	CB_CHECKF(samplers.get_size() == 0, "Some samplers have not been freed before deleting the device!");

	/** We can't do this yet as some dtor depends on current_device (get_device()) */
	//current_device = nullptr;	
}

Device::Frame::Frame() : gfx_command_pool(QueueType::Gfx),
	compute_command_pool(QueueType::Compute)
{
	auto fence = get_device()->create_fence(FenceCreateInfo());
	gfx_fence = fence.get_value();
}

void Device::Frame::free_resources()
{
	for(auto& buffer : expired_buffers)
		get_device()->buffers.free(cast_handle<Buffer>(buffer));

	for(auto& texture : expired_textures)
		get_device()->textures.free(cast_handle<Texture>(texture));

	for(auto& texture_view : expired_texture_views)
		get_device()->texture_views.free(cast_handle<TextureView>(texture_view));

	for(auto& shader : expired_shaders)
		get_device()->shaders.free(cast_handle<Shader>(shader));

	for(auto& pipeline_layout : expired_pipeline_layouts)
		get_device()->pipeline_layouts.free(cast_handle<PipelineLayout>(pipeline_layout));

	for(auto& swapchain : expired_swapchains)
		get_device()->swapchains.free(cast_handle<Swapchain>(swapchain));

	for(auto& fence : expired_fences)
		get_device()->fences.free(cast_handle<Fence>(fence));
	
	for(auto& semaphore : expired_semaphores)
		get_device()->semaphores.free(cast_handle<Semaphore>(semaphore));

	for(auto& sampler : expired_samplers)
		get_device()->samplers.free(cast_handle<Sampler>(sampler));
}

void Device::wait_idle()
{
	backend_device->wait_idle();
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
		get_current_frame().gfx_submitted = true;
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

cb::Result<BufferHandle, Result> Device::create_buffer(BufferInfo in_create_info)
{
	in_create_info.info.usage_flags |= BufferUsageFlagBits::TransferSrc | BufferUsageFlagBits::TransferDst;
	auto result = backend_device->create_buffer(in_create_info.info);
	if(!result)
		return result.get_error();

	auto buffer = buffers.allocate(*this, result.get_value(), in_create_info.debug_name);
	auto handle = cast_resource_ptr<BufferHandle>(buffer);
	
	if(!in_create_info.initial_data.empty())
	{
		if(in_create_info.info.mem_usage != MemoryUsage::CpuOnly)
		{
			auto staging = create_buffer(BufferInfo::make_staging(in_create_info.info.size,
				in_create_info.initial_data).set_debug_name("Copy Staging Buffer (create_buffer)"));
			if(!staging)
			{
				destroy_buffer(handle);
				return result.get_error();
			}

			// TODO: Transfer queue
			auto list = allocate_cmd_list(QueueType::Gfx);

			std::array regions = { BufferCopyRegion(0, 0, in_create_info.info.size) };
			cmd_copy_buffer(list, staging.get_value(), handle, regions);
			destroy_buffer(staging.get_value());
			submit(list);
		}
		else
		{
			auto map = backend_device->map_buffer(result.get_value());
			if(!map)
			{
				destroy_buffer(handle);
				return make_error(map.get_error());
			}

			memcpy(map.get_value(), in_create_info.initial_data.data(), in_create_info.initial_data.size());
			backend_device->unmap_buffer(result.get_value());
		}
	}
	
	return make_result(handle);
}

cb::Result<TextureHandle, Result> Device::create_texture(TextureInfo in_create_info)
{
	in_create_info.info.usage_flags |= TextureUsageFlagBits::TransferSrc | TextureUsageFlagBits::TransferDst;
	auto result = backend_device->create_texture(in_create_info.info);
	if(!result)
		return result.get_error();

	auto texture = textures.allocate(*this, 
		in_create_info.info, 
		false,
		result.get_value(), 
		in_create_info.debug_name);
	auto handle = cast_resource_ptr<TextureHandle>(texture);

	if(!in_create_info.initial_data.empty())
	{
		CB_CHECKF(format_to_aspect_flags(in_create_info.info.format) == TextureAspectFlags(TextureAspectFlagBits::Color),
			"Only color texture formats support uploading initial data !");

		/** Create a staging buffer containing our initial data */
		UniqueBuffer staging(create_buffer(BufferInfo::make_staging(in_create_info.initial_data.size(),
			in_create_info.initial_data).set_debug_name("Copy Staging Buffer (create_texture)")).get_value());

		/** Transition our fresh texture to TransferDst */
		auto list = allocate_cmd_list(QueueType::Gfx);
		std::array regions =
		{
			BufferTextureCopyRegion(0, 
				TextureSubresourceLayers(format_to_aspect_flags(texture->get_create_info().format),
				0,
				0,
				1),
				Offset3D(),
				Extent3D(texture->get_create_info().width, 
				texture->get_create_info().height,
				texture->get_create_info().depth))
		};
		cmd_texture_barrier(list,
			handle,
			PipelineStageFlags(PipelineStageFlagBits::TopOfPipe),
			TextureLayout::Undefined,
			AccessFlags(),
			PipelineStageFlags(PipelineStageFlagBits::Transfer),
			TextureLayout::TransferDst,
			AccessFlags(AccessFlagBits::TransferWrite));
		cmd_copy_buffer_to_texture(list,
			staging.get(),
			handle,
			TextureLayout::TransferDst,
			regions);
		cmd_texture_barrier(list,
			handle,
			PipelineStageFlags(PipelineStageFlagBits::Transfer),
			TextureLayout::TransferDst,
			AccessFlags(AccessFlagBits::TransferWrite),
			PipelineStageFlags(PipelineStageFlagBits::FragmentShader),
			TextureLayout::ShaderReadOnly,
			AccessFlags(AccessFlagBits::ShaderRead));
		submit(list);
	}

	return make_result(handle);
}

cb::Result<TextureViewHandle, Result> Device::create_texture_view(TextureViewInfo in_create_info)
{
	CB_CHECK(in_create_info.texture);

	auto texture = cast_handle<Texture>(in_create_info.texture);
	TextureViewCreateInfo create_info(
		texture->get_resource(),
		in_create_info.type,
		in_create_info.format,
		in_create_info.subresource_range);
	auto result = backend_device->create_texture_view(create_info);
	if(!result)
		return result.get_error();

	auto texture_view = texture_views.allocate(*this, 
		*texture, 
		create_info,
		result.get_value(), 
		in_create_info.debug_name);
	return make_result(cast_resource_ptr<TextureViewHandle>(texture_view));
}

cb::Result<ShaderHandle, Result> Device::create_shader(const ShaderInfo& in_create_info)
{
	auto result = backend_device->create_shader(in_create_info.create_info);
	if(!result)
		return result.get_error();

	auto shader = shaders.allocate(*this, result.get_value(), in_create_info.debug_name);
	return make_result(cast_resource_ptr<ShaderHandle>(shader));
}

cb::Result<SwapchainHandle, Result> Device::create_swapchain(const SwapChainInfo& in_create_info)
{
	auto result = backend_device->create_swap_chain(in_create_info.create_info);
	if(!result)
		return result.get_error();

	auto swapchain = swapchains.allocate(*this, in_create_info.create_info, result.get_value(), in_create_info.debug_name);
	return make_result(cast_resource_ptr<SwapchainHandle>(swapchain));
}

cb::Result<FenceHandle, Result> Device::create_fence(const FenceInfo& in_create_info)
{
	auto result = backend_device->create_fence(in_create_info.create_info);
	if(!result)
		return result.get_error();

	auto fence = fences.allocate(*this, result.get_value(), in_create_info.debug_name);
	return make_result(cast_resource_ptr<FenceHandle>(fence));
}

cb::Result<SemaphoreHandle, Result> Device::create_semaphore(const SemaphoreInfo& in_create_info)
{
	auto result = backend_device->create_semaphore(in_create_info.create_info);
	if(!result)
		return result.get_error();

	auto semaphore = semaphores.allocate(*this, result.get_value(), in_create_info.debug_name);
	return make_result(cast_resource_ptr<SemaphoreHandle>(semaphore));
}

cb::Result<PipelineLayoutHandle, Result> Device::create_pipeline_layout(const PipelineLayoutInfo& in_create_info)
{
	auto result = backend_device->create_pipeline_layout(in_create_info.create_info);
	if(!result)
		return result.get_error();

	auto layout = pipeline_layouts.allocate(*this, result.get_value(), in_create_info.debug_name);
	return make_result(cast_resource_ptr<PipelineLayoutHandle>(layout));
}

cb::Result<SamplerHandle, Result> Device::create_sampler(const SamplerInfo& in_create_info)
{
	auto result = backend_device->create_sampler(in_create_info.create_info);
	if(!result)
		return result.get_error();

	auto sampler = samplers.allocate(*this, result.get_value(), in_create_info.debug_name);
	return make_result(cast_resource_ptr<SamplerHandle>(sampler));
}

void Device::destroy_buffer(const BufferHandle& in_buffer)
{
	get_current_frame().expired_buffers.emplace_back(in_buffer);
}

void Device::destroy_texture(const TextureHandle& in_texture)
{
	get_current_frame().expired_textures.emplace_back(in_texture);
}

void Device::destroy_texture_view(const TextureViewHandle& in_texture_view)
{
	get_current_frame().expired_texture_views.emplace_back(in_texture_view);
}

void Device::destroy_sampler(const SamplerHandle& in_sampler)
{
	get_current_frame().expired_samplers.emplace_back(in_sampler);
}

void Device::destroy_shader(const ShaderHandle& in_shader)
{
	get_current_frame().expired_shaders.emplace_back(in_shader);
}

void Device::destroy_swapchain(const SwapchainHandle& in_swapchain)
{
	get_current_frame().expired_swapchains.emplace_back(in_swapchain);
}

void Device::destroy_pipeline_layout(const PipelineLayoutHandle& in_pipeline_layout)
{
	get_current_frame().expired_pipeline_layouts.emplace_back(in_pipeline_layout);
}

void Device::destroy_fence(const FenceHandle& in_fence)
{
	get_current_frame().expired_fences.emplace_back(in_fence);
}

void Device::destroy_semaphore(const SemaphoreHandle& in_semaphore)
{
	get_current_frame().expired_semaphores.emplace_back(in_semaphore);
}

cb::Result<void*, Result> Device::map_buffer(const BufferHandle& in_handle)
{
	return backend_device->map_buffer(cast_handle<Buffer>(in_handle)->get_resource());	
}

void Device::unmap_buffer(const BufferHandle& in_handle)
{
	backend_device->unmap_buffer(cast_handle<Buffer>(in_handle)->get_resource());	
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
		attachments.emplace_back(view->get_resource());
		attachment_descriptions.emplace_back(view->get_create_info().format,
			view->get_texture().get_create_info().sample_count,
			AttachmentLoadOp::Clear,
			AttachmentStoreOp::Store,
			AttachmentLoadOp::DontCare,
			AttachmentStoreOp::DontCare,
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

		AttachmentReference depth_stencil_attachment;

		/** Depth/stencil attachment always at the end ! */
		if(in_info.depth_stencil_attachment)
		{
			depth_stencil_attachment = AttachmentReference(static_cast<uint32_t>(attachment_descriptions.size()) - 1,
				subpass.mode == RenderPassInfo::DepthStencilMode::ReadWrite 
					? TextureLayout::DepthStencilAttachment : TextureLayout::DepthReadOnly);
		}
		
		subpasses.push_back(SubpassDescription(input_attachments,
			color_attachments,
			resolve_attachments,
			depth_stencil_attachment,
			{}));
	}
	
	auto render_pass = get_or_create_render_pass(RenderPassCreateInfo(attachment_descriptions, subpasses));
	auto list = cast_handle<CommandList>(in_cmd_list);

	list->set_render_pass(render_pass);

	framebuffer.attachments = attachments;
	
	backend_device->cmd_begin_render_pass(
		list->get_resource(),
		render_pass,
		framebuffer,
		in_info.render_area,
		in_info.clear_values);

	std::array viewports = { Viewport(0, 0, 
		static_cast<float>(framebuffer.width), static_cast<float>(framebuffer.height), 0.f, 1.f )};
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
	list->prepare_draw();

	backend_device->cmd_draw(list->get_resource(),
		in_vertex_count,
		in_instance_count,
		in_first_vertex,
		in_first_index);	
}

void Device::cmd_draw_indexed(const CommandListHandle& in_list, 
	const uint32_t in_index_count,
	const uint32_t in_instance_count, 
	const uint32_t in_first_index, 
	const int32_t in_vertex_offset, 
	const uint32_t in_first_instance)
{
	auto list = cast_handle<CommandList>(in_list);
	list->prepare_draw();

	backend_device->cmd_draw_indexed(list->get_resource(),
		in_index_count,
		in_instance_count,
		in_first_index,
		in_vertex_offset,
		in_first_instance);
}

void Device::cmd_set_render_pass_state(const CommandListHandle& in_cmd_list, 
	const PipelineRenderPassState& in_state)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	list->set_render_pass_state(in_state);
}

void Device::cmd_set_material_state(const CommandListHandle& in_cmd_list, 
	const PipelineMaterialState& in_state)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	list->set_material_state(in_state);
}

void Device::cmd_set_scissor(const CommandListHandle& in_cmd_list, const Rect2D& in_scissor)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	std::array scissors = { in_scissor };
	backend_device->cmd_set_scissors(list->get_resource(), 0, scissors);
}

void Device::cmd_bind_pipeline_layout(const CommandListHandle& in_cmd_list, 
	const PipelineLayoutHandle& in_handle)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	list->set_pipeline_layout(in_handle);
}

void Device::cmd_bind_ubo(const CommandListHandle& in_cmd_list, 
	const uint32_t in_set, 
	const uint32_t in_binding, 
	const BufferHandle& in_handle)
{
	auto list = cast_handle<CommandList>(in_cmd_list);
	list->set_descriptor(in_set, in_binding, in_handle ? Descriptor::make_buffer_info(DescriptorType::UniformBuffer,
		in_binding,
		in_handle ? cast_handle<Buffer>(in_handle)->get_resource() : null_backend_resource) : Descriptor());
}

void Device::cmd_bind_texture_view(const CommandListHandle& in_cmd_list, 
	const uint32_t in_set, 
	const uint32_t in_binding, 
	const TextureViewHandle& in_handle)
{
	auto list = cast_handle<CommandList>(in_cmd_list);

	list->set_descriptor(in_set, in_binding, in_handle ? Descriptor::make_texture_view_info(in_binding,
		in_handle ? cast_handle<TextureView>(in_handle)->get_resource() : null_backend_resource) : Descriptor());
}

void Device::cmd_bind_sampler(const CommandListHandle& in_cmd_list, 
	const uint32_t in_set, 
	const uint32_t in_binding, 
	const SamplerHandle& in_handle)
{
	CB_CHECK(in_handle);

	auto list = cast_handle<CommandList>(in_cmd_list);
	list->set_descriptor(in_set, in_binding, in_handle ? Descriptor::make_sampler_info(in_binding,
		in_handle ? cast_handle<Sampler>(in_handle)->get_resource() : null_backend_resource) : Descriptor());
}

void Device::cmd_end_render_pass(const CommandListHandle& in_cmd_list)
{
	backend_device->cmd_end_render_pass(cast_handle<CommandList>(in_cmd_list)->get_resource());
}

void Device::cmd_bind_vertex_buffer(const CommandListHandle& in_cmd_list, const BufferHandle& in_buffer, const uint64_t in_offset)
{
	std::array vertex_buffers = { cast_handle<Buffer>(in_buffer)->get_resource() };
	std::array offsets = { in_offset };
	backend_device->cmd_bind_vertex_buffers(cast_handle<CommandList>(in_cmd_list)->get_resource(),
		0,
		vertex_buffers,
		offsets);	
}

void Device::cmd_bind_index_buffer(const CommandListHandle& in_cmd_list, 
	const BufferHandle& in_buffer, 
	const uint64_t in_offset, 
	const IndexType in_index_type)
{
	backend_device->cmd_bind_index_buffer(cast_handle<CommandList>(in_cmd_list)->get_resource(),
		cast_handle<Buffer>(in_buffer)->get_resource(),
		in_offset,
		in_index_type);
}

void Device::cmd_copy_buffer(const CommandListHandle& in_cmd_list, 
	const BufferHandle& in_src_buffer, 
	const BufferHandle& in_dst_buffer, 
	const std::span<BufferCopyRegion>& in_regions)
{
	CB_CHECK(!in_regions.empty());
	CB_CHECK(in_src_buffer);
	CB_CHECK(in_dst_buffer);

	auto list = cast_handle<CommandList>(in_cmd_list);
	auto src = cast_handle<Buffer>(in_src_buffer);
	auto dst = cast_handle<Buffer>(in_dst_buffer);
	
	backend_device->cmd_copy_buffer(list->get_resource(),
		src->get_resource(),
		dst->get_resource(),
		in_regions);	
}

void Device::cmd_copy_buffer_to_texture(const CommandListHandle& in_cmd_list, 
	const BufferHandle& in_src_buffer, 
	const TextureHandle& in_dst_texture,
	const TextureLayout in_dst_layout,
	const std::span<BufferTextureCopyRegion>& in_regions)
{
	CB_CHECK(!in_regions.empty());
	CB_CHECK(in_src_buffer);
	CB_CHECK(in_dst_texture);

	auto list = cast_handle<CommandList>(in_cmd_list);
	auto src = cast_handle<Buffer>(in_src_buffer);
	auto dst = cast_handle<Texture>(in_dst_texture);

	backend_device->cmd_copy_buffer_to_texture(list->get_resource(),
		src->get_resource(),
		dst->get_resource(),
		in_dst_layout,
		in_regions);
}

void Device::cmd_texture_barrier(const CommandListHandle& in_cmd_list,
	const TextureHandle& in_texture,
	const PipelineStageFlags in_src_flags, 
	const TextureLayout in_src_layout, 
	const AccessFlags in_src_access_flags, 
	const PipelineStageFlags in_dst_flags, 
	const TextureLayout in_dst_layout, 
	const AccessFlags in_dst_access_flags)
{
	CB_CHECK(in_texture);

	auto texture = cast_handle<Texture>(in_texture);

	std::array barriers = 
	{
		TextureMemoryBarrier(texture->get_resource(),
			in_src_access_flags,
			in_dst_access_flags,
			in_src_layout,
			in_dst_layout,
			TextureSubresourceRange(format_to_aspect_flags(texture->get_create_info().format), 
				0, texture->get_create_info().mip_levels,
				0, texture->get_create_info().array_layers))
	};

	backend_device->cmd_pipeline_barrier(cast_handle<CommandList>(in_cmd_list)->get_resource(),
		in_src_flags,
		in_dst_flags,
		barriers);
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

BackendDeviceResource Device::get_swapchain_backend_handle(const SwapchainHandle& in_swapchain) const
{
	return cast_handle<Swapchain>(in_swapchain)->get_resource();
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