#pragma once

#include "Vulkan.hpp"

namespace cb::gfx
{

class VulkanPipeline final
{
public:
	VulkanPipeline(VulkanDevice& in_device, 
		VkPipeline in_pipeline) : device(in_device), pipeline(in_pipeline) {}

	VulkanPipeline(VulkanPipeline&& in_other) noexcept = delete;
	
	~VulkanPipeline()
	{
		vkDestroyPipeline(device.get_device(), pipeline, nullptr);	
	}
private:
	VulkanDevice& device;
	VkPipeline pipeline;
};

inline VkVertexInputRate convert_vertex_input_rate(const VertexInputRate& in_rate)
{
	switch(in_rate)
	{
	default:
	case VertexInputRate::Vertex:
		return VK_VERTEX_INPUT_RATE_VERTEX;
	case VertexInputRate::Instance:
		return VK_VERTEX_INPUT_RATE_INSTANCE;
	}
}

inline VkPrimitiveTopology convert_primitive_topology(const PrimitiveTopology& in_topo)
{
	switch(in_topo)
	{
	default:
	case PrimitiveTopology::TriangleList:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case PrimitiveTopology::TriangleStrip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case PrimitiveTopology::TriangleFan:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	case PrimitiveTopology::LineList:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case PrimitiveTopology::LineStrip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case PrimitiveTopology::PointList:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case PrimitiveTopology::PatchList:
		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	}
}

inline VkPolygonMode convert_polygon_mode(const PolygonMode& in_polygon_mode)
{
	switch(in_polygon_mode)
	{
	default:
	case PolygonMode::Fill:
		return VK_POLYGON_MODE_FILL;
	case PolygonMode::Line:
		return VK_POLYGON_MODE_LINE;
	}
}

inline VkCullModeFlagBits convert_cull_mode(const CullMode& in_cull_mode)
{
	switch(in_cull_mode)
	{
	default:
	case CullMode::None:
		return VK_CULL_MODE_NONE;
	case CullMode::Back:
		return VK_CULL_MODE_BACK_BIT;
	case CullMode::Front:
		return VK_CULL_MODE_FRONT_BIT;
	case CullMode::FrontAndBack:
		return VK_CULL_MODE_FRONT_AND_BACK;
	}
}

inline VkFrontFace convert_front_face(const FrontFace& in_front_face)
{
	switch(in_front_face)
	{
	default:
	case FrontFace::Clockwise:
		return VK_FRONT_FACE_CLOCKWISE;
	case FrontFace::CounterClockwise:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

inline VkCompareOp convert_compare_op(const CompareOp& in_op)
{
	switch(in_op)
	{
	default:
	case CompareOp::Never:
		return VK_COMPARE_OP_NEVER;
	case CompareOp::NotEqual:
		return VK_COMPARE_OP_NOT_EQUAL;
	case CompareOp::Less:
		return VK_COMPARE_OP_LESS;
	case CompareOp::LessOrEqual:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case CompareOp::Equal:
		return VK_COMPARE_OP_EQUAL;
	case CompareOp::Greater:
		return VK_COMPARE_OP_GREATER;
	case CompareOp::GreaterOrEqual:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case CompareOp::Always:
		return VK_COMPARE_OP_ALWAYS;
	}
}

inline VkStencilOp convert_stencil_op(const StencilOp& in_op)
{
	switch(in_op)
	{
	default:
	case StencilOp::Keep:
		return VK_STENCIL_OP_KEEP;
	case StencilOp::Zero:
		return VK_STENCIL_OP_ZERO;
	case StencilOp::Replace:
		return VK_STENCIL_OP_REPLACE;
	case StencilOp::IncrementAndClamp:
		return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case StencilOp::DecrementAndClamp:
		return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case StencilOp::Invert:
		return VK_STENCIL_OP_INVERT;
	case StencilOp::IncrementAndWrap:
		return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case StencilOp::DecrementAndWrap:
		return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	}
}
	
inline VkStencilOpState convert_stencil_op_state(const StencilOpState& in_state)
{
	VkStencilOpState state;
	state.failOp = convert_stencil_op(in_state.fail_op);
	state.passOp = convert_stencil_op(in_state.pass_op);
	state.depthFailOp = convert_stencil_op(in_state.depth_fail_op);
	state.compareOp = convert_compare_op(in_state.compare_op);
	state.compareMask = in_state.compare_mask;
	state.writeMask = in_state.write_mask;
	state.reference = in_state.reference;
	return state;
}

inline VkLogicOp convert_logic_op(const LogicOp& in_op)
{
	switch(in_op)
	{
	default:
	case LogicOp::Clear:
		return VK_LOGIC_OP_CLEAR;
	case LogicOp::And:
		return VK_LOGIC_OP_AND;
	case LogicOp::AndReverse:
		return VK_LOGIC_OP_AND_REVERSE;
	case LogicOp::AndInverted:
		return VK_LOGIC_OP_AND_INVERTED;
	case LogicOp::Copy:
		return VK_LOGIC_OP_COPY;
	case LogicOp::CopyInverted:
		return VK_LOGIC_OP_COPY_INVERTED;
	case LogicOp::NoOp:
		return VK_LOGIC_OP_NO_OP;
	case LogicOp::Or:
		return VK_LOGIC_OP_OR;
	case LogicOp::Equivalent:
		return VK_LOGIC_OP_EQUIVALENT;
	case LogicOp::Invert:
		return VK_LOGIC_OP_INVERT;
	case LogicOp::OrReverse:
		return VK_LOGIC_OP_OR_REVERSE;
	case LogicOp::OrInverted:
		return VK_LOGIC_OP_OR_INVERTED;
	case LogicOp::Nand:
		return VK_LOGIC_OP_NAND;
	case LogicOp::Set:
		return VK_LOGIC_OP_SET;
	}
}

inline VkBlendFactor convert_blend_factor(const BlendFactor& in_factor)
{
	switch(in_factor)
	{
	default:
	case BlendFactor::Zero:
		return VK_BLEND_FACTOR_ZERO;
	case BlendFactor::ConstantAlpha:
		return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case BlendFactor::ConstantColor:
		return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case BlendFactor::OneMinusConstantAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	case BlendFactor::OneMinusConstantColor:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case BlendFactor::One:
		return VK_BLEND_FACTOR_ONE;
	case BlendFactor::SrcColor:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case BlendFactor::SrcAlpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case BlendFactor::DstColor:
		return VK_BLEND_FACTOR_DST_COLOR;
	case BlendFactor::DstAlpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case BlendFactor::OneMinusSrcColor:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case BlendFactor::OneMinusDstColor:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case BlendFactor::OneMinusSrcAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BlendFactor::OneMinusDstAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	}
}

inline VkBlendOp convert_blend_op(const BlendOp& in_op)
{
	switch(in_op)
	{
	default:
	case BlendOp::Add:
		return VK_BLEND_OP_ADD;
	case BlendOp::Min:
		return VK_BLEND_OP_MIN;
	case BlendOp::Max:
		return VK_BLEND_OP_MAX;
	case BlendOp::Substract:
		return VK_BLEND_OP_SUBTRACT;
	case BlendOp::ReverseSubstract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	}
}

inline VkColorComponentFlags convert_color_component_flags(const ColorComponentFlags& in_flags)
{
	VkColorComponentFlags flags = 0;

	if(in_flags & ColorComponentFlagBits::R)
		flags |= VK_COLOR_COMPONENT_R_BIT;
	
	if(in_flags & ColorComponentFlagBits::G)
		flags |= VK_COLOR_COMPONENT_G_BIT;
	
	if(in_flags & ColorComponentFlagBits::B)
		flags |= VK_COLOR_COMPONENT_B_BIT;
	
	if(in_flags & ColorComponentFlagBits::A)
		flags |= VK_COLOR_COMPONENT_A_BIT;
	
	return flags;
}
	
}