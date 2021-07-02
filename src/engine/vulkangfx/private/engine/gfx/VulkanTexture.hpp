#pragma once

#include "Vulkan.hpp"

namespace cb::gfx
{

inline VkSampleCountFlagBits convert_sample_count_bit(const SampleCountFlagBits& in_bit)
{
	switch(in_bit)
	{
	default:
	case SampleCountFlagBits::Count1:
		return VK_SAMPLE_COUNT_1_BIT;
	case SampleCountFlagBits::Count2:
		return VK_SAMPLE_COUNT_2_BIT;
	case SampleCountFlagBits::Count4:
		return VK_SAMPLE_COUNT_4_BIT;
	case SampleCountFlagBits::Count8:
		return VK_SAMPLE_COUNT_8_BIT;
	case SampleCountFlagBits::Count16:
		return VK_SAMPLE_COUNT_16_BIT;
	case SampleCountFlagBits::Count32:
		return VK_SAMPLE_COUNT_32_BIT;
	case SampleCountFlagBits::Count64:
		return VK_SAMPLE_COUNT_64_BIT;
	}
}
	
}