#pragma once

namespace cb::gfx
{

struct SemaphoreCreateInfo
{

};

struct FenceCreateInfo
{
	bool signaled;

	FenceCreateInfo(const bool in_signaled = false) : signaled(in_signaled) {}
};

}