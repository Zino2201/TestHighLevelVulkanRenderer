#pragma once

#include "engine/Flags.hpp"
#include "engine/gfx/DeviceResource.hpp"
#include "engine/Hash.hpp"
#include "Texture.hpp"

namespace cb::gfx
{

enum class PipelineStageFlagBits
{
	TopOfPipe = 1 << 0,
	InputAssembler = 1 << 1,
	VertexShader = 1 << 2,
	TessellationControlShader = 1 << 3,
	TessellationEvaluationShader = 1 << 4,
	GeometryShader = 1 << 5,
	EarlyFragmentTests = 1 << 6,
	FragmentShader = 1 << 7,
	LateFragmentTests = 1 << 8,
	ColorAttachmentOutput = 1 << 9,
	ComputeShader = 1 << 10,
	Transfer = 1 << 11,
	BottomOfPipe = 1 << 12,

	/** Graphics stages */
	AllGraphics = InputAssembler | VertexShader | TessellationControlShader | 
		TessellationEvaluationShader | GeometryShader | FragmentShader | EarlyFragmentTests | LateFragmentTests | ColorAttachmentOutput,
};
CB_ENABLE_FLAG_ENUMS(PipelineStageFlagBits, PipelineStageFlags);

enum class ShaderStageFlagBits
{
	Vertex = 1 << 0,
	TessellationControl = 1 << 1,
	TessellationEvaluation = 1 << 2,
	Geometry = 1 << 3,
	Fragment = 1 << 4,
	Compute = 1 << 5,
};
CB_ENABLE_FLAG_ENUMS(ShaderStageFlagBits, ShaderStageFlags);
	
/**
 * A single shader stage of a pipeline
 */
struct PipelineShaderStage
{
	ShaderStageFlagBits shader_stage;
	BackendDeviceResource shader;
	const char* entry_point;

	PipelineShaderStage(const ShaderStageFlagBits& in_shader_stage,
		const BackendDeviceResource& in_shader,
		const char* in_entry_point) : shader_stage(in_shader_stage),
		shader(in_shader), entry_point(in_entry_point) {}

	bool operator==(const PipelineShaderStage& in_other) const
	{
		return shader_stage == in_other.shader_stage &&
			shader == in_other.shader &&
			entry_point == in_other.entry_point;
	}
};

enum class PipelineBindPoint
{
	Gfx,
	Compute
};

struct Viewport
{
	float x;
	float y;
	float width;
	float height;
	float min_depth;
	float max_depth;

	explicit Viewport(const float in_x = 0.f,
		const float in_y = 0.f,
		const float in_width = 0.f,
		const float in_height = 0.f,
		const float in_min_depth = 0.f,
		const float in_max_depth = 1.f) : x(in_x), y(in_y),
		width(in_width), height(in_height), min_depth(in_min_depth), max_depth(in_max_depth) {}
};

/** Barrier related structures */
enum class AccessFlagBits
{
	TransferRead = 1 << 0,
	TransferWrite = 1 << 1,
	ShaderRead = 1 << 2,
	ShaderWrite = 1 << 3,
	HostRead = 1 << 4,
	HostWrite = 1 << 5,
	MemoryRead = 1 << 6,
	MemoryWrite = 1 << 7,
	ColorAttachmentRead = 1 << 8,
	ColorAttachmentWrite = 1 << 9,
	DepthStencilAttachmentRead = 1 << 10,
	DepthStencilAttachmentWrite = 1 << 11,
	InputAttachmentRead = 1 << 11,
	UniformRead = 1 << 12,
};
CB_ENABLE_FLAG_ENUMS(AccessFlagBits, AccessFlags);

struct TextureMemoryBarrier
{
	BackendDeviceResource texture;
	AccessFlags src_access_flags;
	AccessFlags dst_access_flags;
	TextureLayout old_layout;
	TextureLayout new_layout;
	TextureSubresourceRange subresource_range;

	TextureMemoryBarrier(const BackendDeviceResource in_texture,
		AccessFlags in_src_access_flags,
		AccessFlags in_dst_access_flags,
		TextureLayout in_old_layout,
		TextureLayout in_new_layout,
		const TextureSubresourceRange& in_subresource_range) :
		texture(in_texture), src_access_flags(in_src_access_flags), dst_access_flags(in_dst_access_flags),
		old_layout(in_old_layout), new_layout(in_new_layout),
		subresource_range(in_subresource_range) {}
};

}

namespace std
{

template<> struct hash<cb::gfx::PipelineShaderStage>
{
	uint64_t operator()(const cb::gfx::PipelineShaderStage& in_stage) const noexcept
	{
		uint64_t hash = 0;

		cb::hash_combine(hash, in_stage.shader_stage);
		cb::hash_combine(hash, in_stage.shader);
		cb::hash_combine(hash, in_stage.entry_point);

		return hash;
	}
};	

}