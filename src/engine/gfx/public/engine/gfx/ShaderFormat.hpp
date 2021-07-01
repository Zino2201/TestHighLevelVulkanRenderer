#pragma once

namespace cb::gfx
{

/**
 * A shader model is a set of GPU features
 * This enum uses the D3D shader model format as it is the most used
 */
enum class ShaderModel
{
	/** Tessellation, wave ops ... */
	SM6_0,

	/** Ray tracing, mesh/amplifications shaders */
	SM6_5,
};

enum class ShaderLanguage
{
	VK_SPIRV,
	DXIL,
	MSL
};

struct ShaderFormat
{
	
};
	
}