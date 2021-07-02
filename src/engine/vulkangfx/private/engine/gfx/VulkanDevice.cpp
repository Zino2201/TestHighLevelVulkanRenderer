#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanShader.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanTexture.hpp"
#include "VulkanPipelineLayout.hpp"

namespace cb::gfx
{

VulkanDevice::VulkanDevice(VulkanBackend& in_backend, vkb::Device&& in_device) :
	backend(in_backend),
	device_wrapper(DeviceWrapper(std::move(in_device))),
	surface_manager(*this),
	allocator(nullptr)
{
	VmaAllocatorCreateInfo create_info = {};
	create_info.instance = backend.get_instance();
	create_info.device = device_wrapper.device.device;
	create_info.physicalDevice = device_wrapper.device.physical_device.physical_device;

	// TODO: Error check
	vmaCreateAllocator(&create_info, &allocator);
}
	
VulkanDevice::~VulkanDevice() = default;

cb::Result<BackendDeviceResource, Result> VulkanDevice::create_buffer(const BufferCreateInfo& in_create_info)
{
	CB_CHECK(in_create_info.size != 0 && in_create_info.usage_flags != BufferUsageFlags())
	if(in_create_info.size == 0 || in_create_info.usage_flags == BufferUsageFlags())
		return make_error(Result::ErrorInvalidParameter);

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = nullptr;
	buffer_create_info.size = in_create_info.size;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = nullptr;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.usage = 0;
	buffer_create_info.flags = 0;

	/** Usage flags */
	if(in_create_info.usage_flags & BufferUsageFlagBits::VertexBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::IndexBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::UniformBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	
	if(in_create_info.usage_flags & BufferUsageFlagBits::StorageBuffer)
		buffer_create_info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	
	VmaAllocationCreateInfo alloc_create_info = {};
	alloc_create_info.flags = 0;
	alloc_create_info.usage = convert_memory_usage(in_create_info.mem_usage);
	
	VmaAllocation allocation;
	VkBuffer handle;
	
	VkResult result = vmaCreateBuffer(allocator, 
		&buffer_create_info, 
		&alloc_create_info,
		&handle,
		&allocation,
		nullptr);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto buffer = new_resource<VulkanBuffer>(*this, handle, allocation);
	return make_result(buffer.get());
}
	
cb::Result<BackendDeviceResource, Result> VulkanDevice::create_swap_chain(const SwapChainCreateInfo& in_create_info)
{
	CB_CHECK(in_create_info.os_handle && in_create_info.width != 0 && in_create_info.height != 0);
	if(!in_create_info.os_handle || in_create_info.width == 0 || in_create_info.height == 0)
		return make_error(Result::ErrorInvalidParameter);

	auto surface = surface_manager.get_or_create(in_create_info.os_handle);
	if(!surface)
	{
		return make_error(surface.get_error());
	}
	
	auto swapchain = new_resource<VulkanSwapChain>(*this, surface.get_value());
	VkResult result = swapchain->create(in_create_info.width, in_create_info.height);
	if(result != VK_SUCCESS)
	{
		free_resource<VulkanSwapChain>(swapchain);
		return make_error(convert_result(result));
	}
	
	return static_cast<BackendDeviceResource>(swapchain);
}

cb::Result<BackendDeviceResource, Result> VulkanDevice::create_shader(const ShaderCreateInfo& in_create_info)
{
	CB_CHECK(!in_create_info.bytecode.empty());
	if(in_create_info.bytecode.empty())
		return make_error(Result::ErrorInvalidParameter);

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.codeSize = in_create_info.bytecode.size();
	create_info.pCode = in_create_info.bytecode.data();
	create_info.flags = 0;

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(get_device(),
		&create_info,
		nullptr,
		&shader_module);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto shader = new_resource<VulkanShader>(*this, shader_module);
	return make_result(shader.get());
}

cb::Result<BackendDeviceResource, Result> VulkanDevice::create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info)
{
	VkGraphicsPipelineCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = nullptr;

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
	shader_stages.reserve(in_create_info.shader_stages.size());
	for(const auto& stage : in_create_info.shader_stages)
	{
		VkPipelineShaderStageCreateInfo stage_create_info = {};
		stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info.pNext = nullptr;
		stage_create_info.stage = convert_shader_stage_bits(stage.shader_stage);
		stage_create_info.module = get_resource<VulkanShader>(stage.shader)->shader_module;
		stage_create_info.pName = stage.entry_point;
		stage_create_info.flags = 0;
		stage_create_info.pSpecializationInfo = nullptr;
		shader_stages.push_back(stage_create_info);
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.pNext = nullptr;

	std::vector<VkVertexInputBindingDescription> bindings;
	bindings.reserve(in_create_info.vertex_input_state.input_binding_descriptions.size());
	for(const auto& input_binding_desc : in_create_info.vertex_input_state.input_binding_descriptions)
	{
		VkVertexInputBindingDescription binding_desc = {};
		binding_desc.binding = input_binding_desc.binding;
		binding_desc.stride = input_binding_desc.stride;
		binding_desc.inputRate = convert_vertex_input_rate(input_binding_desc.input_rate);
		bindings.push_back(binding_desc);
	}

	std::vector<VkVertexInputAttributeDescription> attributes = {};
	attributes.reserve(in_create_info.vertex_input_state.input_attribute_descriptions.size());
	for(const auto& input_attribute_desc : in_create_info.vertex_input_state.input_attribute_descriptions)
	{
		VkVertexInputAttributeDescription attribute_desc = {};
		attribute_desc.binding = input_attribute_desc.binding;
		attribute_desc.location = input_attribute_desc.location;
		attribute_desc.offset = input_attribute_desc.offset;
		attribute_desc.format = convert_format(input_attribute_desc.format);
		attributes.push_back(attribute_desc);
	}

	vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
	vertex_input_state.pVertexBindingDescriptions = bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(bindings.size());
	vertex_input_state.pVertexAttributeDescriptions = attributes.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = nullptr;
	input_assembly_state.flags = 0;
	input_assembly_state.topology = convert_primitive_topology(in_create_info.input_assembly_state.primitive_topology);
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterization_state = {};
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = nullptr;
	rasterization_state.polygonMode = convert_polygon_mode(in_create_info.rasterization_state.polygon_mode);
	rasterization_state.cullMode = convert_cull_mode(in_create_info.rasterization_state.cull_mode);
	rasterization_state.frontFace = convert_front_face(in_create_info.rasterization_state.front_face);
	rasterization_state.depthClampEnable = in_create_info.rasterization_state.enable_depth_clamp;
	rasterization_state.depthBiasEnable = in_create_info.rasterization_state.enable_depth_bias;
	rasterization_state.depthBiasConstantFactor = in_create_info.rasterization_state.depth_bias_constant_factor;
	rasterization_state.depthBiasClamp = in_create_info.rasterization_state.depth_bias_clamp;
	rasterization_state.depthBiasSlopeFactor = in_create_info.rasterization_state.depth_bias_slope_factor;

	VkPipelineMultisampleStateCreateInfo multisampling_state = {};
	multisampling_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_state.pNext = nullptr;
	multisampling_state.rasterizationSamples = convert_sample_count_bit(in_create_info.multisampling_state.samples);
	multisampling_state.sampleShadingEnable = VK_FALSE;
	multisampling_state.minSampleShading = 0.f;
	multisampling_state.pSampleMask = nullptr;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.pNext = nullptr;
	depth_stencil_state.depthTestEnable = in_create_info.depth_stencil_state.enable_depth_test;
	depth_stencil_state.depthWriteEnable = in_create_info.depth_stencil_state.enable_depth_write;
	depth_stencil_state.depthCompareOp = convert_compare_op(in_create_info.depth_stencil_state.depth_compare_op);
	depth_stencil_state.depthBoundsTestEnable = in_create_info.depth_stencil_state.enable_depth_bounds_test;
	depth_stencil_state.stencilTestEnable = in_create_info.depth_stencil_state.enable_stencil_test;
	depth_stencil_state.front = convert_stencil_op_state(in_create_info.depth_stencil_state.front_face);
	depth_stencil_state.back = convert_stencil_op_state(in_create_info.depth_stencil_state.back_face);

	VkPipelineColorBlendStateCreateInfo color_blend_state = {};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.logicOpEnable = in_create_info.color_blend_state.enable_logic_op;
	color_blend_state.logicOp = convert_logic_op(in_create_info.color_blend_state.logic_op);

	std::vector<VkPipelineColorBlendAttachmentState> color_attachments;
	color_attachments.reserve(in_create_info.color_blend_state.attachments.size());
	for(const auto& color_attachment : in_create_info.color_blend_state.attachments)
	{
		VkPipelineColorBlendAttachmentState state = {};
		state.blendEnable = color_attachment.enable_blend;
		state.srcColorBlendFactor = convert_blend_factor(color_attachment.src_color_blend_factor);
		state.dstColorBlendFactor = convert_blend_factor(color_attachment.dst_color_blend_factor);
		state.colorBlendOp = convert_blend_op(color_attachment.color_blend_op);
		state.srcAlphaBlendFactor = convert_blend_factor(color_attachment.src_alpha_blend_factor);
		state.dstAlphaBlendFactor = convert_blend_factor(color_attachment.dst_alpha_blend_factor);
		state.alphaBlendOp = convert_blend_op(color_attachment.alpha_blend_op);
		state.colorWriteMask = convert_color_component_flags(color_attachment.color_write_flags);
		color_attachments.push_back(state);
	}
	color_blend_state.attachmentCount = static_cast<uint32_t>(color_attachments.size());
	color_blend_state.pAttachments = color_attachments.data();
	color_blend_state.blendConstants[0] = 0.f;
	color_blend_state.blendConstants[1] = 0.f;
	color_blend_state.blendConstants[2] = 0.f;
	color_blend_state.blendConstants[3] = 0.f;

	std::array dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.pNext = nullptr;
	dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state_create_info.pDynamicStates = dynamic_states.data();
	
	create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	create_info.pStages = shader_stages.data();
	create_info.pVertexInputState = &vertex_input_state;
	create_info.pInputAssemblyState = &input_assembly_state;
	create_info.pTessellationState = nullptr;
	create_info.pViewportState = nullptr;
	create_info.pRasterizationState = &rasterization_state;
	create_info.pMultisampleState = &multisampling_state;
	create_info.pDepthStencilState = &depth_stencil_state;
	create_info.pColorBlendState = &color_blend_state;
	create_info.pDynamicState = &dynamic_state_create_info;
	create_info.subpass = in_create_info.subpass;

	VkPipeline pipeline;
	VkResult result = vkCreateGraphicsPipelines(get_device(),
		VK_NULL_HANDLE,
		1,
		&create_info,
		nullptr,
		&pipeline);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	auto ret = new_resource<VulkanPipeline>(*this, pipeline);
	return make_result(ret.get());
}
	
cb::Result<BackendDeviceResource, Result> VulkanDevice::create_render_pass(const RenderPassCreateInfo& in_create_info)
{
	
}

void VulkanDevice::destroy_buffer(const BackendDeviceResource& in_buffer)
{
	free_resource<VulkanBuffer>(in_buffer);
}

void VulkanDevice::destroy_swap_chain(const BackendDeviceResource& in_swap_chain)
{
	free_resource<VulkanSwapChain>(in_swap_chain);
}

void VulkanDevice::destroy_shader(const BackendDeviceResource& in_shader)
{
	free_resource<VulkanShader>(in_shader);
}

void VulkanDevice::destroy_gfx_pipeline(const BackendDeviceResource& in_shader)
{
	
}
	
void VulkanDevice::destroy_render_pass(const BackendDeviceResource& in_shader)
{
	
}

/** Buffers */
cb::Result<void*, Result> VulkanDevice::map_buffer(const BackendDeviceResource& in_buffer)
{
	void* data = nullptr;

	auto buffer = get_resource<VulkanBuffer>(in_buffer);
	VkResult result = vmaMapMemory(allocator,
		buffer->get_allocation(),
		&data);
	if(result != VK_SUCCESS)
		return make_error(convert_result(result));

	return make_result(data);
}

void VulkanDevice::unmap_buffer(const BackendDeviceResource& in_buffer)
{
	auto buffer = get_resource<VulkanBuffer>(in_buffer);
	vmaUnmapMemory(allocator, buffer->get_allocation());
}
	
}