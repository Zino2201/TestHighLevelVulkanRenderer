#pragma once

#include "engine/Core.hpp"

namespace cb::gfx
{

/**
 * Backend device resource handle. Typically a pointer but maybe anything.
 */
using BackendDeviceResource = uint64_t;

enum class DeviceResourceType : uint8_t
{
	Buffer,
	Texture,
	TextureView,
	SwapChain,
	Sampler,
	Shader,
	
	/* Max = 255 */
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
	static constexpr uint64_t null = std::numeric_limits<uint64_t>::max();

	constexpr explicit DeviceResource(const uint64_t in_handle = null) noexcept : handle(in_handle) {}

	constexpr bool operator==(const DeviceResource& in_other) const noexcept { return handle == in_other.handle; }
	constexpr bool operator!=(const DeviceResource& in_other) const noexcept { return handle != in_other.handle; }

	[[nodiscard]] constexpr uint64_t get_handle() const { return handle >> 56; }

	constexpr explicit operator bool() const { return handle != null; }
private:
	uint64_t handle;
};

}
	
	
using SwapChainHandle = detail::DeviceResource<DeviceResourceType::SwapChain>;

}