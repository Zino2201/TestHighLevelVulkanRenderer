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

	
}

namespace std
{

inline std::string to_string(const cb::gfx::ShaderModel& in_shader_model)
{
	switch(in_shader_model)
	{
	case cb::gfx::ShaderModel::SM6_0:
		return "SM6_0";
	case cb::gfx::ShaderModel::SM6_5:
		return "SM6_5";
	}
}

inline std::string to_string(const cb::gfx::ShaderLanguage& in_language)
{
	switch(in_language)
	{
	case cb::gfx::ShaderLanguage::VK_SPIRV:
		return "VK_SPIRV";
	case cb::gfx::ShaderLanguage::DXIL:
		return "DXIL";
	case cb::gfx::ShaderLanguage::MSL:
		return "MSL";
	}
}

}