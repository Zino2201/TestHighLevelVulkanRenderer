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
	CommandList,
	Pipeline,
	PipelineLayout,
	SwapChain,
	Sampler,
	Shader,
	Fence
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

	constexpr bool operator==(const DeviceResource& in_other) const noexcept { return handle == in_other.handle; }
	constexpr bool operator!=(const DeviceResource& in_other) const noexcept { return handle != in_other.handle; }

	[[nodiscard]] constexpr uint64_t get_handle() const { return handle; }

	constexpr explicit operator bool() const { return handle != null; }
private:
	uint64_t handle;
};

}
	
	
using ShaderHandle = detail::DeviceResource<DeviceResourceType::Shader>;
using CommandListHandle = detail::DeviceResource<DeviceResourceType::CommandList>;
using PipelineLayoutHandle = detail::DeviceResource<DeviceResourceType::PipelineLayout>;
using PipelineHandle = detail::DeviceResource<DeviceResourceType::Pipeline>;
using SwapchainHandle = detail::DeviceResource<DeviceResourceType::SwapChain>;
using FenceHandle = detail::DeviceResource<DeviceResourceType::Fence>;

}