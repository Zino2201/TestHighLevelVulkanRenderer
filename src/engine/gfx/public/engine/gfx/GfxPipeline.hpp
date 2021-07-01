#pragma once

#include "Pipeline.hpp"
#include "Format.hpp"
#include "Texture.hpp"

namespace cb::gfx
{

enum class VertexInputRate
{
	Vertex,
	Instance
};

	/**
 * Description of a vertex buffer input binding
 */
struct VertexInputBindingDescription
{
	uint32_t binding;
	uint32_t stride;
	VertexInputRate input_rate;

	VertexInputBindingDescription(const uint32_t in_binding,
		const uint32_t in_stride, const VertexInputRate in_input_rate) 
		: binding(in_binding), stride(in_stride), input_rate(in_input_rate) {}

	bool operator==(const VertexInputBindingDescription& in_other) const
	{
		return binding == in_other.binding &&
			stride == in_other.stride &&
			input_rate == in_other.input_rate;
	}
};

/**
 * Description of a single binding (how the backend interpret it)
 */
struct VertexInputAttributeDescription
{
	uint32_t location;
	uint32_t binding;
	Format format;
	uint32_t offset;

	VertexInputAttributeDescription(const uint32_t in_location,
		const uint32_t in_binding, const Format in_format,
		const uint32_t in_offset) : location(in_location),
		binding(in_binding), format(in_format), offset(in_offset) {}

	bool operator==(const VertexInputAttributeDescription& in_other) const
	{
		return location == in_other.location &&
			binding == in_other.binding &&
			format == in_other.format &&
			offset == in_other.offset;
	}
};

struct PipelineVertexInputStateCreateInfo
{
	std::vector<VertexInputBindingDescription> input_binding_descriptions;
	std::vector<VertexInputAttributeDescription> input_attribute_descriptions;

	PipelineVertexInputStateCreateInfo(const std::vector<VertexInputBindingDescription>& in_input_binding_descriptions = {},
		const std::vector<VertexInputAttributeDescription>& in_input_attribute_descriptions = {})
		: input_binding_descriptions(in_input_binding_descriptions), input_attribute_descriptions(in_input_attribute_descriptions) {}

	bool operator==(const PipelineVertexInputStateCreateInfo& in_other) const
	{
		return input_binding_descriptions == in_other.input_binding_descriptions &&
			input_attribute_descriptions == in_other.input_attribute_descriptions;
	}
};

enum class PolygonMode
{
	Fill,
	Line
};

enum class CullMode
{
	None,
	Front,
	Back,
	FrontAndBack
};

enum class FrontFace
{
	CounterClockwise,
	Clockwise
};

/**
 * Describe the rasterizer state
 */
struct PipelineRasterizationStateCreateInfo
{
	PolygonMode polygon_mode;
	CullMode cull_mode;
	FrontFace front_face;
	bool enable_depth_clamp;
	bool enable_depth_bias;
	float depth_bias_constant_factor;
	float depth_bias_clamp;
	float depth_bias_slope_factor;

	PipelineRasterizationStateCreateInfo(
		const PolygonMode in_polygon_mode = PolygonMode::Fill,
		const CullMode in_cull_mode = CullMode::None, 
		const FrontFace in_front_face = FrontFace::CounterClockwise,
		const bool in_enable_depth_clamp = 0.0f, 
		const bool in_enable_depth_bias = false,
		const float in_depth_bias_constant_factor = 0.0f, 
		const float in_depth_bias_clamp = 0.0f,
		const float in_depth_bias_slope_factor = 0.0f) : polygon_mode(in_polygon_mode),
		cull_mode(in_cull_mode), front_face(in_front_face), enable_depth_clamp(in_enable_depth_clamp),
		enable_depth_bias(in_enable_depth_bias), depth_bias_constant_factor(in_depth_bias_constant_factor),
		depth_bias_clamp(in_depth_bias_clamp), depth_bias_slope_factor(in_depth_bias_slope_factor) {}

	bool operator==(const PipelineRasterizationStateCreateInfo& in_other) const
	{
		return polygon_mode == in_other.polygon_mode &&
			cull_mode == in_other.cull_mode &&
			front_face == in_other.front_face &&
			enable_depth_clamp == in_other.enable_depth_clamp &&
			enable_depth_bias == in_other.enable_depth_bias &&
			depth_bias_constant_factor == in_other.depth_bias_constant_factor &&
			depth_bias_clamp == in_other.depth_bias_clamp &&
			depth_bias_slope_factor == in_other.depth_bias_slope_factor;
	}
};

struct PipelineMultisamplingStateCreateInfo
{
	SampleCountFlagBits samples;

	PipelineMultisamplingStateCreateInfo(const SampleCountFlagBits& in_samples = SampleCountFlagBits::Count1)
		: samples(in_samples) {}

	bool operator==(const PipelineMultisamplingStateCreateInfo& in_other) const
	{
		return samples == in_other.samples;
	}
};

enum class CompareOp
{
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always
};

enum class StencilOp
{
	Keep,
	Zero,
	Replace,
	IncrementAndClamp,
	DecrementAndClamp,
	Invert,
	IncrementAndWrap,
	DecrementAndWrap
};

struct StencilOpState
{
	StencilOp fail_op;
	StencilOp pass_op;
	StencilOp depth_fail_op;
	CompareOp compare_op;
	uint32_t compare_mask;
	uint32_t write_mask;
	uint32_t reference;

	StencilOpState(const StencilOp in_fail_op = StencilOp::Zero,
		const StencilOp in_pass_op = StencilOp::Zero,
		const StencilOp in_depth_fail_op = StencilOp::Zero,
		const CompareOp in_compare_op = CompareOp::Never,
		const uint32_t in_compare_mask = 0,
		const uint32_t in_write_mask = 0,
		const uint32_t in_reference = 0) : fail_op(in_fail_op),
		pass_op(in_pass_op), depth_fail_op(in_depth_fail_op),
		compare_op(in_compare_op), compare_mask(in_compare_mask),
		write_mask(in_write_mask), reference(in_reference) {}

	bool operator==(const StencilOpState& in_other) const
	{
		return fail_op == in_other.fail_op &&
			pass_op == in_other.pass_op &&
			depth_fail_op == in_other.depth_fail_op &&
			compare_op == in_other.compare_op &&
			compare_mask == in_other.compare_mask &&
			write_mask == in_other.write_mask &&
			reference == in_other.reference;
	}
};

struct PipelineDepthStencilStateCreateInfo
{
	bool enable_depth_test;
	bool enable_depth_write;
	CompareOp depth_compare_op;
	bool enable_depth_bounds_test;
	bool enable_stencil_test;
	StencilOpState front_face;
	StencilOpState back_face;

	PipelineDepthStencilStateCreateInfo(const bool in_enable_depth_test = false,
		const bool in_enable_depth_write = false,
		const CompareOp in_compare_op = CompareOp::Never,
		const bool in_enable_depth_bounds_test = false,
		const bool in_enable_stencil_test = false,
		const StencilOpState in_front_face = StencilOpState(),
		const StencilOpState in_back_face = StencilOpState()) : enable_depth_test(in_enable_depth_test),
		enable_depth_write(in_enable_depth_write), depth_compare_op(in_compare_op),
		enable_depth_bounds_test(in_enable_depth_bounds_test), enable_stencil_test(in_enable_stencil_test),
		front_face(in_front_face), back_face(in_back_face) {}

	bool operator==(const PipelineDepthStencilStateCreateInfo& in_other) const
	{
		return enable_depth_test == in_other.enable_depth_test &&
			enable_depth_write == in_other.enable_depth_write &&
			depth_compare_op == in_other.depth_compare_op &&
			enable_depth_bounds_test == in_other.enable_depth_bounds_test &&
			enable_stencil_test == in_other.enable_stencil_test &&
			front_face == in_other.front_face &&
			back_face == in_other.back_face;
	}
};

static constexpr size_t max_attachments_per_framebuffer = 8;

enum class BlendFactor
{
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
	ConstantColor,
	OneMinusConstantColor,
	ConstantAlpha,
	OneMinusConstantAlpha,
};

enum class BlendOp
{
	Add,
	Substract,
	ReverseSubstract,
	Min,
	Max
};

enum class ColorComponentFlagBits
{
	R = 1 << 0,
	G = 1 << 1,
	B = 1 << 2,
	A = 1 << 3,

	RGBA = R | G | B | A
};
CB_ENABLE_FLAG_ENUMS(ColorComponentFlagBits, ColorComponentFlags);

/**
 * Color blend attachment of a single attachment
 */
struct PipelineColorBlendAttachmentState
{
	bool enable_blend;
	BlendFactor src_color_blend_factor;
	BlendFactor dst_color_blend_factor;
	BlendOp color_blend_op;
	BlendFactor src_alpha_blend_factor;
	BlendFactor dst_alpha_blend_factor;
	BlendOp alpha_blend_op;
	ColorComponentFlags color_write_flags;

	PipelineColorBlendAttachmentState(const bool in_enable_blend = false,
		const BlendFactor in_src_color_blend_factor = BlendFactor::One,
		const BlendFactor in_dst_color_blend_factor = BlendFactor::Zero,
		const BlendOp in_color_blend_op = BlendOp::Add,
		const BlendFactor in_src_alpha_blend_factor = BlendFactor::One,
		const BlendFactor in_dst_alpha_blend_factor = BlendFactor::Zero,
		const BlendOp in_alpha_blend_op = BlendOp::Add,
		const ColorComponentFlags in_color_write_flags = ColorComponentFlagBits::RGBA) :
		enable_blend(in_enable_blend), src_color_blend_factor(in_src_color_blend_factor),
		dst_color_blend_factor(in_dst_color_blend_factor), color_blend_op(in_color_blend_op),
		src_alpha_blend_factor(in_src_alpha_blend_factor), dst_alpha_blend_factor(in_dst_alpha_blend_factor),
		alpha_blend_op(in_alpha_blend_op), color_write_flags(in_color_write_flags) {}

	bool operator==(const PipelineColorBlendAttachmentState& in_other) const
	{
		return enable_blend == in_other.enable_blend &&
			src_color_blend_factor == in_other.src_color_blend_factor &&
			dst_color_blend_factor == in_other.dst_color_blend_factor &&
			color_blend_op == in_other.color_blend_op &&
			src_alpha_blend_factor == in_other.src_alpha_blend_factor &&
			dst_alpha_blend_factor == in_other.dst_alpha_blend_factor &&
			alpha_blend_op == in_other.alpha_blend_op &&
			color_write_flags == in_other.color_write_flags;
	}
};

enum class LogicOp
{
	Clear,
	And,
	AndReverse,
	Copy,
	AndInverted,
	NoOp,
	Xor,
	Or,
	Nor,
	Equivalent,
	Invert,
	OrReverse,
	CopyInverted,
	OrInverted,
	Nand,
	Set
};

struct PipelineColorBlendStateCreationInfo
{
	bool enable_logic_op;
	LogicOp logic_op;
	std::array<PipelineColorBlendAttachmentState, max_attachments_per_framebuffer> attachment_states;

	PipelineColorBlendStateCreationInfo(const bool in_enable_logic_op = false,
		LogicOp in_logic_op = LogicOp::NoOp,
		const std::array<PipelineColorBlendAttachmentState, max_attachments_per_framebuffer>& in_attachment_states = {})
		: enable_logic_op(in_enable_logic_op), logic_op(in_logic_op),
		attachment_states(in_attachment_states) {}

	bool operator==(const PipelineColorBlendStateCreationInfo& in_other) const
	{
		return enable_logic_op == in_other.enable_logic_op &&
			logic_op == in_other.logic_op &&
			attachment_states == in_other.attachment_states;
	}
};

enum class PrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	TriangleFan,
	PatchList
};

struct PipelineInputAssemblyStateCreateInfo
{
	PrimitiveTopology primitive_topology;

	PipelineInputAssemblyStateCreateInfo(const PrimitiveTopology in_topology = PrimitiveTopology::TriangleList)
		: primitive_topology(in_topology) {}

	bool operator==(const PipelineInputAssemblyStateCreateInfo& in_other) const
	{
		return primitive_topology == in_other.primitive_topology;
	}
};

struct GfxPipelineCreateInfo
{
	std::vector<PipelineShaderStage> shader_stages;
	PipelineVertexInputStateCreateInfo vertex_input_state;
	PipelineInputAssemblyStateCreateInfo input_assembly_state;
	PipelineRasterizationStateCreateInfo rasterization_state;
	PipelineMultisamplingStateCreateInfo multisampling_state;
	PipelineDepthStencilStateCreateInfo depth_stencil_state;
	PipelineColorBlendStateCreationInfo color_blend_state;
	BackendDeviceResource pipeline_layout;
	BackendDeviceResource render_pass;

	/** Subpass where this pipeline will be used */
	uint32_t subpass;

	GfxPipelineCreateInfo(const std::vector<PipelineShaderStage>& in_shader_stages = {},
		const PipelineVertexInputStateCreateInfo& in_vertex_input_state = PipelineVertexInputStateCreateInfo(),
		const PipelineInputAssemblyStateCreateInfo& in_input_assembly_state = PipelineInputAssemblyStateCreateInfo(),
		const PipelineRasterizationStateCreateInfo& in_rasterization_state = PipelineRasterizationStateCreateInfo(),
		const PipelineMultisamplingStateCreateInfo& in_multisampling_state = PipelineMultisamplingStateCreateInfo(),
		const PipelineDepthStencilStateCreateInfo& in_depth_stencil_state = PipelineDepthStencilStateCreateInfo(),
		const PipelineColorBlendStateCreationInfo& in_color_blend_state = PipelineColorBlendStateCreationInfo(),
		const BackendDeviceResource& in_pipeline_layout = BackendDeviceResource(),
		const BackendDeviceResource& in_render_pass = BackendDeviceResource(),
		const uint32_t& in_subpass = 0) :
		shader_stages(in_shader_stages), vertex_input_state(in_vertex_input_state),
		input_assembly_state(in_input_assembly_state), rasterization_state(in_rasterization_state),
		multisampling_state(in_multisampling_state), depth_stencil_state(in_depth_stencil_state),
		color_blend_state(in_color_blend_state), pipeline_layout(in_pipeline_layout), render_pass(in_render_pass), subpass(in_subpass) {}

	bool operator==(const GfxPipelineCreateInfo& in_create_info) const
	{
		return shader_stages == in_create_info.shader_stages &&
			vertex_input_state == in_create_info.vertex_input_state &&
			input_assembly_state == in_create_info.input_assembly_state &&
			rasterization_state == in_create_info.rasterization_state &&
			multisampling_state == in_create_info.multisampling_state &&
			depth_stencil_state == in_create_info.depth_stencil_state &&
			color_blend_state == in_create_info.color_blend_state &&
			pipeline_layout == in_create_info.pipeline_layout &&
			render_pass == in_create_info.render_pass &&
			subpass == in_create_info.subpass;
	}
};
	
}