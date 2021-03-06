#pragma once

#include "engine/Core.hpp"
#include <limits>

namespace cb::gfx
{

/**
 * Backend device resource handle. Typically a pointer but maybe anything.
 */
using BackendDeviceResource = uint64_t;

static constexpr BackendDeviceResource null_backend_resource = 0;

enum class DeviceResourceType : uint8_t
{
	Buffer,
	Texture,
	TextureView,
	CommandPool,
	CommandList,
	Pipeline,
	PipelineLayout,
	Swapchain,
	Sampler,
	Shader,
	Fence,
	Semaphore
};

namespace detail
{
	
/**
 * Compile-time type-safe (using an enum) device handle
 * Used only for the high-level gfx API as it should be used instead of the low-level one (BackendXXXX classes)
 */
template<DeviceResourceType Type>
struct DeviceResource
{
	static constexpr uint64_t null = ~0Ui64;

	constexpr explicit DeviceResource(const uint64_t in_handle = null) noexcept : handle(in_handle) {}

	constexpr DeviceResource(const DeviceResource& in_handle) : handle(in_handle.get_handle()) {}
	constexpr DeviceResource& operator=(const DeviceResource& in_handle)
	{
		handle = in_handle.get_handle();
		return *this;
	}

	constexpr DeviceResource(DeviceResource&& in_handle) noexcept : handle(std::exchange(in_handle.handle, null)) {}
	constexpr DeviceResource& operator=(DeviceResource&& in_handle) noexcept
	{
		handle = std::exchange(in_handle.handle, null);
		return *this;
	}

	constexpr bool operator==(const DeviceResource& in_other) const noexcept { return handle == in_other.handle; }
	constexpr bool operator!=(const DeviceResource& in_other) const noexcept { return handle != in_other.handle; }

	[[nodiscard]] constexpr uint64_t get_handle() const { return handle; }

	[[nodiscard]] constexpr explicit operator bool() const { return handle != null; }
private:
	uint64_t handle;
};

}
	
	
using BufferHandle = detail::DeviceResource<DeviceResourceType::Buffer>;
using TextureHandle = detail::DeviceResource<DeviceResourceType::Texture>;
using TextureViewHandle = detail::DeviceResource<DeviceResourceType::TextureView>;
using ShaderHandle = detail::DeviceResource<DeviceResourceType::Shader>;
using CommandListHandle = detail::DeviceResource<DeviceResourceType::CommandList>;
using PipelineLayoutHandle = detail::DeviceResource<DeviceResourceType::PipelineLayout>;
using PipelineHandle = detail::DeviceResource<DeviceResourceType::Pipeline>;
using SwapchainHandle = detail::DeviceResource<DeviceResourceType::Swapchain>;
using FenceHandle = detail::DeviceResource<DeviceResourceType::Fence>;
using SemaphoreHandle = detail::DeviceResource<DeviceResourceType::Semaphore>;
using SamplerHandle = detail::DeviceResource<DeviceResourceType::Sampler>;

}

namespace std
{

inline std::string to_string(const cb::gfx::DeviceResourceType& in_type)
{
	switch(in_type)
	{
	default:
		return "Unknown";
	case cb::gfx::DeviceResourceType::Buffer:
		return "Buffer";
	case cb::gfx::DeviceResourceType::Texture:
		return "Texture";
	case cb::gfx::DeviceResourceType::TextureView:
		return "TextureView";
	case cb::gfx::DeviceResourceType::Sampler:
		return "Sampler";
	case cb::gfx::DeviceResourceType::Swapchain:
		return "Swapchain";
	case cb::gfx::DeviceResourceType::Pipeline:
		return "Pipeline";
	case cb::gfx::DeviceResourceType::Shader:
		return "Shader";
	case cb::gfx::DeviceResourceType::CommandList:
		return "CommandList";
	case cb::gfx::DeviceResourceType::Fence:
		return "Fence";
	case cb::gfx::DeviceResourceType::PipelineLayout:
		return "PipelineLayout";
	case cb::gfx::DeviceResourceType::Semaphore:
		return "Semaphore";
	}
}

}