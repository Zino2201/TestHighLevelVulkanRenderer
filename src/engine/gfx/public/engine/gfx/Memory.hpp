#pragma once

namespace cb::gfx
{

enum class MemoryUsage
{
	CpuOnly,
	GpuOnly,
	CpuToGpu,
	GpuToCpu
};
	
}