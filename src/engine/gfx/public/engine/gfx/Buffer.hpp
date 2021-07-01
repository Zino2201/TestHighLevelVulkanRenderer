#pragma once

#include "engine/Core.hpp"
#include "engine/Flags.hpp"
#include "Memory.hpp"

namespace cb::gfx
{
	
enum class BufferUsageFlagBits
{
	VertexBuffer = 1 << 0,
	IndexBuffer = 1 << 1,
	UniformBuffer = 1 << 2,
	StorageBuffer = 1 << 3,
};
CB_ENABLE_FLAG_ENUMS(BufferUsageFlagBits, BufferUsageFlags);
	
struct BufferCreateInfo
{
	uint64_t size;
	MemoryUsage mem_usage;
	BufferUsageFlags usage_flags;

	BufferCreateInfo() : size(0), mem_usage(MemoryUsage::CpuOnly), usage_flags(BufferUsageFlags()) {}
};
	
}