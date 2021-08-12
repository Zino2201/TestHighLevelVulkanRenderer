#pragma once

#include "engine/gfx/Backend.hpp"
#if CB_VULKAN_GFX_PRIVATE
#include "engine/gfx/Vulkan.hpp"
#endif

namespace cb::gfx
{

#if CB_VULKAN_GFX_PRIVATE
class VulkanBackend final : public Backend
{
public:
	VulkanBackend(const BackendFlags& in_flags);
	~VulkanBackend();
	
	cb::Result<std::unique_ptr<BackendDevice>, std::string> create_device(ShaderModel in_requested_shader_model) override;

	[[nodiscard]] bool is_initialized() const
	{
		return instance.instance != VK_NULL_HANDLE;
	}
	
	[[nodiscard]] const std::string& get_error() const { return error; }
	[[nodiscard]] VkInstance get_instance() const { return instance.instance; }
	[[nodiscard]] bool has_debug_layers() const { return debug_layers_enabled; }
private:
	vkb::Instance instance;
	std::string error;
	bool debug_layers_enabled;
public:
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
};
#endif

cb::Result<std::unique_ptr<Backend>, std::string> create_vulkan_backend(const BackendFlags& in_flags);
	
}