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
	TransferSrc = 1 << 4,
	TransferDst = 1 << 5,
};
CB_ENABLE_FLAG_ENUMS(BufferUsageFlagBits, BufferUsageFlags);
	
struct BufferCreateInfo
{
	uint64_t size;
	MemoryUsage mem_usage;
	BufferUsageFlags usage_flags;

	BufferCreateInfo(uint64_t in_size = 0,
		MemoryUsage in_mem_usage = MemoryUsage::CpuOnly,
		BufferUsageFlags in_usage_flags = BufferUsageFlags())
		: size(in_size), mem_usage(in_mem_usage), usage_flags(in_usage_flags) {}
};
	
}