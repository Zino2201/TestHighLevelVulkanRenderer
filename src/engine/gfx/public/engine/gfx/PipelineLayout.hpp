#pragma once

#include <span>
#include <variant>
#include "Pipeline.hpp"
#include "Texture.hpp"

namespace cb::gfx
{

enum class DescriptorType
{
	UniformBuffer,
	Sampler,
	SampledTexture,
	StorageTexture,
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

	DescriptorSetLayoutCreateInfo(const std::span<DescriptorSetLayoutBinding>& in_bindings = {}) : bindings(in_bindings) {}
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

struct DescriptorBufferInfo
{
	BackendDeviceResource handle;
	uint64_t offset;
	uint64_t range;

	DescriptorBufferInfo(const BackendDeviceResource in_handle,
		const uint64_t in_offset,
		const uint64_t in_range) : handle(in_handle),
		offset(in_offset),
		range(in_range) {}
};

struct DescriptorTextureInfo
{
	BackendDeviceResource texture_view;
	TextureLayout layout;

	DescriptorTextureInfo(const BackendDeviceResource in_texture_view,
		const TextureLayout in_layout) : texture_view(in_texture_view),
		layout(in_layout) {}
};

struct DescriptorSamplerInfo
{
	BackendDeviceResource sampler;

	DescriptorSamplerInfo(const BackendDeviceResource in_sampler) : sampler(in_sampler) {}
};

struct Descriptor
{
	enum
	{
		None,
		BufferInfo,
		TextureInfo,
		SamplerInfo
	};

	DescriptorType type;
	uint32_t binding;
	std::variant<std::monostate, DescriptorBufferInfo, DescriptorTextureInfo, DescriptorSamplerInfo> info;

	Descriptor() : type(DescriptorType::UniformBuffer),
		binding(0),
		info(std::monostate{}) {}

	static Descriptor make_buffer_info(DescriptorType in_type,
		const uint32_t in_binding, 
		const BackendDeviceResource in_handle)
	{
		Descriptor descriptor;
		descriptor.type = in_type;
		descriptor.binding = in_binding;
		descriptor.info = DescriptorBufferInfo(in_handle, 0, std::numeric_limits<uint64_t>::max());
		return descriptor;
	}

	static Descriptor make_texture_view_info(const uint32_t in_binding,
		const BackendDeviceResource in_view)
	{
		Descriptor descriptor;
		descriptor.type = DescriptorType::SampledTexture;
		descriptor.binding = in_binding;
		descriptor.info = DescriptorTextureInfo(in_view,
			TextureLayout::ShaderReadOnly);
		return descriptor;
	}

	static Descriptor make_sampler_info(const uint32_t in_binding,
		const BackendDeviceResource in_sampler)
	{
		Descriptor descriptor;
		descriptor.type = DescriptorType::Sampler;
		descriptor.binding = in_binding;
		descriptor.info = DescriptorSamplerInfo(in_sampler);
		return descriptor;
	}
};

struct PipelineLayoutCreateInfo
{
	std::span<DescriptorSetLayoutCreateInfo> set_layouts;
	std::span<PushConstantRange> push_constant_ranges;

	PipelineLayoutCreateInfo(const std::span<DescriptorSetLayoutCreateInfo>& in_set_layouts,
		const std::span<PushConstantRange>& in_push_constant_ranges = {}) : set_layouts(in_set_layouts),
	push_constant_ranges(in_push_constant_ranges) {}
};

static constexpr int max_descriptor_sets = 4;
static constexpr int max_bindings = 16;

}

namespace std
{

template<> struct hash<cb::gfx::DescriptorBufferInfo>
{
	uint64_t operator()(const cb::gfx::DescriptorBufferInfo& in_info) const noexcept
	{
		uint64_t hash = 0;
		cb::hash_combine(hash, in_info.handle);
		cb::hash_combine(hash, in_info.offset);
		cb::hash_combine(hash, in_info.range);
		return hash;
	}
};

template<> struct hash<cb::gfx::DescriptorTextureInfo>
{
	uint64_t operator()(const cb::gfx::DescriptorTextureInfo& in_info) const noexcept
	{
		uint64_t hash = 0;
		cb::hash_combine(hash, in_info.texture_view);
		cb::hash_combine(hash, in_info.layout);
		return hash;
	}
};

template<> struct hash<cb::gfx::DescriptorSamplerInfo>
{
	uint64_t operator()(const cb::gfx::DescriptorSamplerInfo& in_info) const noexcept
	{
		uint64_t hash = 0;
		cb::hash_combine(hash, in_info.sampler);
		return hash;
	}
};

template<> struct hash<cb::gfx::Descriptor>
{
	uint64_t operator()(const cb::gfx::Descriptor& in_descriptor) const noexcept
	{
		uint64_t hash = 0;
		cb::hash_combine(hash, in_descriptor.binding);
		cb::hash_combine(hash, in_descriptor.type);
		cb::hash_combine(hash, in_descriptor.info);
		return hash;
	}
};

}