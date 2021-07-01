#pragma once

#include "engine/Core.hpp"

namespace cb::gfx
{

struct SwapChainCreateInfo
{
	void* os_handle;
	uint32_t width;
	uint32_t height;

	SwapChainCreateInfo(void* in_window_handle = nullptr,
		const uint32_t& in_width = 0,
		const uint32_t& in_height = 0) : os_handle(in_window_handle),
		width(in_width), height(in_height) {}
};
	
}