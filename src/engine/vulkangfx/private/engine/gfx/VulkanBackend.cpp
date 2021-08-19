#include "engine/gfx/VulkanBackend.hpp"
#include "engine/gfx/Device.hpp"
#include "VulkanDevice.hpp"

namespace cb::gfx
{

VulkanBackend::VulkanBackend(const BackendFlags& in_flags)
	: Backend(in_flags), debug_layers_enabled(false)
{
	name = "Vulkan";
	shader_language = ShaderLanguage::VK_SPIRV;
	supported_shader_models = { ShaderModel::SM6_0, ShaderModel::SM6_5 };
	
	vkb::InstanceBuilder builder;
	builder.set_app_name("CityBuilder");
	builder.set_engine_name("ze_cb");
	builder.require_api_version(1, 2, 0);
	
	/** Customize our instance based on the flags */
	if(in_flags & BackendFlagBits::DebugLayers)
	{
		debug_layers_enabled = true;
		builder.enable_validation_layers();
		builder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
		builder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
		builder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
		builder.set_debug_callback(
			[](VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
				VkDebugUtilsMessageTypeFlagsEXT message_type,
				const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
				void *user_data) -> VkBool32 
			{
				(void)(user_data);
				
				const auto type = vkb::to_string_message_type(message_type);

				switch(message_severity)
				{
				default:
					logger::verbose(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
					logger::info(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
					logger::warn(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
					logger::error(log_vulkan, "[{}] {}", type, callback_data->pMessage);
					CB_DEBUGBREAK();
					break;
				}
				
				return VK_FALSE;
			}
	    );

	}

	/** Create instance */
	{
		auto result = builder.build();
		if(!result)
		{
			error = fmt::format("Failed to create Vulkan Instance: {}", result.error().message());
			return;
		}

		instance = result.value();
	}

	if(has_debug_layers())
		vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(
		instance.instance, "vkSetDebugUtilsObjectNameEXT"));
}

VulkanBackend::~VulkanBackend()
{
	CB_CHECKF(alive_vulkan_objects == 0, "Some objects have not been destroyed!");

	logger::verbose(log_vulkan, "~VulkanBackend()");

	vkb::destroy_instance(instance);
}
	
cb::Result<std::unique_ptr<BackendDevice>, std::string> VulkanBackend::create_device(ShaderModel in_requested_shader_model)
{
	(void)(in_requested_shader_model);

	vkb::PhysicalDevice physical_device;
	
	/** At this point we have an instance, let's try finding a suitable physical device */
	{
		vkb::PhysicalDeviceSelector phys_device_selector(instance);
		/** We don't have any surfaces yet */
		phys_device_selector.defer_surface_initialization();
		phys_device_selector.require_present();

		VkPhysicalDeviceFeatures required_features = {};
		required_features.fillModeNonSolid = VK_TRUE;
		phys_device_selector.set_required_features(required_features);
		auto result = phys_device_selector.select();
		if(!result)
		{
			return fmt::format("Failed to select a physical device: {}", result.error().message());
		}

		physical_device = result.value();

		logger::info(log_vulkan, "Found suitable GPU \"{}\"", physical_device.properties.deviceName);
	}

	vkb::DeviceBuilder device_builder(physical_device);
	auto device = device_builder.build();
	if(!device)
	{
		return fmt::format("Failed to create logical device: {}", device.error().message());
	}
	
	return make_result(std::make_unique<VulkanDevice>(*this, std::move(device.value())));	
}

cb::Result<std::unique_ptr<Backend>, std::string> create_vulkan_backend(const BackendFlags& in_flags)
{
	auto backend = std::make_unique<VulkanBackend>(in_flags);
	if(backend->is_initialized())
		return make_result(std::move(backend));
	
	return make_error(backend->get_error());
}
	
}