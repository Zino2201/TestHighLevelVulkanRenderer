#pragma once

#include "engine/Core.hpp"
#include "DeviceResource.hpp"
#include "SwapChain.hpp"
#include "engine/Result.hpp"
#include "Result.hpp"
#include "engine/util/SimplePool.hpp"

namespace cb::gfx
{
	
/**
 * A GPU device, used to communicate with it
 * The engine currently only supports one active GPU (as using multiple GPU is hard to manage)
 */
class Device
{
public:
	Device() = default; 
	virtual ~Device() = default;
	
	Device(const Device&) = delete;
	Device(Device&&) noexcept = default;
	 
	void operator=(const Device&) = delete;
	Device& operator=(Device&&) noexcept = default;

	cb::Result<SwapChainHandle, Result> create_swapchain(const SwapChainCreateInfo& in_create_info);
	//void destroy_swapchain()
private:
	/** Resources pools */
	ThreadSafeSimplePool<SwapChainHandle> swapchains;
};
	
/**
 * Get the currently used device
 */
Device* get_device();

/**
 * Smart device resources
 */

namespace detail
{
/**
 * std::unique_ptr-like for DeviceResource
 */
template<DeviceResourceType Type, typename Deleter>
struct UniqueDeviceResource
{
	using HandleType = DeviceResource<Type>;

	UniqueDeviceResource() {}
	UniqueDeviceResource(HandleType&& in_handle) : handle(in_handle) {}
	~UniqueDeviceResource() { if(handle) deleter()(handle); }

	UniqueDeviceResource(const UniqueDeviceResource&) = delete;
	void operator=(const UniqueDeviceResource&) = delete;

	HandleType get() const
	{
		return handle;
	}

	HandleType operator*() const
	{
		return handle;
	}
private:
	HandleType handle;
	[[no_unique_address]] Deleter deleter;
};

/**
 * Deleters
 */
struct SwapchainDeleter
{
	void operator()(const SwapChainHandle& in_handle) const
	{
		
	}
};
	
}

using UniqueSwapchain = detail::UniqueDeviceResource<DeviceResourceType::SwapChain, detail::SwapchainDeleter>;
	
}