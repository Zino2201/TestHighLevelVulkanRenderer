#include "engine/gfx/Window.hpp"
#include <GLFW/glfw3.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#if CB_COMPILER(MSVC)
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#endif

#include "engine/gfx/VulkanBackend.hpp"
#include "engine/gfx/BackendDevice.hpp"
#include "engine/logger/Logger.hpp"

int main()
{
	using namespace cb;

	spdlog::set_default_logger(std::make_shared<Logger>("Default Logger", std::make_shared<spdlog::sinks::stdout_color_sink_mt>()));
	spdlog::set_pattern("[%H:%M:%S] [%^%l%$/Thr %t] %v");
	/** Add VS debugger sink */
#if CB_COMPILER(MSVC)
	auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/CityBuilder.log");
	spdlog::default_logger()->sinks().push_back(std::move(msvc_sink));
	spdlog::default_logger()->sinks().push_back(std::move(file_sink));
#endif
	
	glfwInit();
	
	Window win(1280, 
		720, 
		WindowFlags(WindowFlagBits::Centered));

	auto result = create_vulkan_backend(gfx::BackendFlags(gfx::BackendFlagBits::DebugLayers));
	if(!result)
	{
		spdlog::critical("Failed to create backend: {}", result.get_error());
		return -1;
	}
	
	auto device = result.get_value()->create_device(gfx::ShaderModel::SM6_0);
	if(!device)
	{
		spdlog::critical("Failed to create device: {}", device.get_error());
		return -1;
	}

	auto swap = device.get_value()->create_swap_chain(gfx::SwapChainCreateInfo(win.get_native_handle(), 
		1280, 
		720));
		
	while(!glfwWindowShouldClose(win.get_handle()))
	{
		glfwPollEvents();
	}

	device.get_value()->destroy_swap_chain(swap.get_value());
	
	glfwTerminate();
	
	return 0;
}