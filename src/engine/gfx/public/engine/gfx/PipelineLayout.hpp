#pragma once

#include <span>
#include "Pipeline.hpp"

namespace cb::gfx
{

enum class DescriptorType
{
	UniformBuffer,
	Sampler,
	SampledTexture,
	StorageInput,
	InputAttachment
};

struct DescriptorSetLayoutBinding
{
	uint32_t binding;
	DescriptorType type;
	uint32_t count;
	ShaderStageFlags stage;

	DescriptorSetLayoutBinding(const uint32_t in_binding,
		const DescriptorType in_descriptor_type,
		const uint32_t in_count,
		const ShaderStageFlags in_stage) : binding(in_binding),
		type(in_descriptor_type), count(in_count), stage(in_stage) {}
};

struct DescriptorSetLayoutCreateInfo
{
	std::span<DescriptorSetLayoutBinding> bindings;
};

struct PushConstantRange
{
	ShaderStageFlags stage;
	uint32_t offset;
	uint32_t size;

	PushConstantRange(const ShaderStageFlags in_stage,
		const uint32_t in_offset,
		const uint32_t in_size) : stage(in_stage),
		offset(in_offset), size(in_size) {}
};

struct PipelineLayoutCreateInfo
{
	std::span<DescriptorSetLayoutCreateInfo> set_layouts;
	std::span<PushConstantRange> push_constant_ranges;
};

}