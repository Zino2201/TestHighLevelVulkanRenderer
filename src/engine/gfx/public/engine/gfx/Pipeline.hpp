#pragma once

#include "engine/Flags.hpp"
#include "engine/gfx/DeviceResource.hpp"

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
	FragmentShader = 1 << 6,
	EarlyFragmentTests = 1 << 7,
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
	
}