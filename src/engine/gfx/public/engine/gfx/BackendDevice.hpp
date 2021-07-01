#pragma once

#include "engine/Core.hpp"
#include "engine/Result.hpp"
#include "DeviceResource.hpp"
#include "Result.hpp"
#include "Buffer.hpp"
#include "SwapChain.hpp"
#include "Shader.hpp"
#include "RenderPass.hpp"
#include "GfxPipeline.hpp"

namespace cb::gfx
{

/**
 * Backend implementation of a GPU device
 * This is the low-level version of cb::gfx::Device, it should not be used directly!
 * Every function here must be called at the proper time, it is not the responsibility of the BackendDevice to ensure
 * that the GPU isn't accessing a specific resource !
 */
class BackendDevice
{
public:
	BackendDevice() = default; 
	virtual ~BackendDevice() = default;
	
	BackendDevice(const BackendDevice&) = delete;
	BackendDevice(BackendDevice&&) noexcept = default;
	 
	void operator=(const BackendDevice&) = delete;
	BackendDevice& operator=(BackendDevice&&) noexcept = default;

	[[nodiscard]] virtual cb::Result<BackendDeviceResource, Result> create_buffer(const BufferCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual cb::Result<BackendDeviceResource, Result> create_swap_chain(const SwapChainCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual cb::Result<BackendDeviceResource, Result> create_shader(const ShaderCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual cb::Result<BackendDeviceResource, Result> create_gfx_pipeline(const GfxPipelineCreateInfo& in_create_info) = 0;
	[[nodiscard]] virtual cb::Result<BackendDeviceResource, Result> create_render_pass(const RenderPassCreateInfo& in_create_info) = 0;
	virtual void destroy_buffer(const BackendDeviceResource& in_swap_chain) = 0;
	virtual void destroy_swap_chain(const BackendDeviceResource& in_swap_chain) = 0;
	virtual void destroy_shader(const BackendDeviceResource& in_shader) = 0;
	virtual void destroy_gfx_pipeline(const BackendDeviceResource& in_shader) = 0;
	virtual void destroy_render_pass(const BackendDeviceResource& in_shader) = 0;

	/** Buffer */
	[[nodiscard]] virtual cb::Result<void*, Result> map_buffer(const BackendDeviceResource& in_buffer) = 0;
	virtual void unmap_buffer(const BackendDeviceResource& in_buffer) = 0;
};
	
}