#include "engine/gfx/Window.hpp"
#include <GLFW/glfw3.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#if CB_COMPILER(MSVC)
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#endif

#include "engine/gfx/VulkanBackend.hpp"
#include "engine/gfx/BackendDevice.hpp"
#include "engine/gfx/Device.hpp"
#include "engine/logger/Logger.hpp"
#include <fstream>

std::vector<char> read_binary_file(const std::string& in_name)
{
	std::ifstream file(in_name, std::ios::ate | std::ios::binary);

	const size_t file_size = file.tellg();
	
	std::vector<char> buffer(file_size);
	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

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

	auto backend_device = result.get_value()->create_device(gfx::ShaderModel::SM6_0);
	if(!backend_device)
	{
		spdlog::critical("Failed to create device: {}", backend_device.get_error());
		return -1;
	}
	
	auto device = std::make_unique<gfx::Device>(*result.get_value().get(), std::move(backend_device.get_value()));

	using namespace gfx;
	
	auto swapchain = device->create_swapchain(gfx::SwapChainCreateInfo(
		win.get_native_handle(),
		1280,
		720));

	auto vert_spv = read_binary_file("vert.spv");
	auto frag_spv = read_binary_file("frag.spv");

	auto vert_shader = device->create_shader(ShaderCreateInfo({ (uint32_t*) vert_spv.data(), vert_spv.size() }));
	auto frag_shader = device->create_shader(ShaderCreateInfo({ (uint32_t*) frag_spv.data(), frag_spv.size() }));

	auto image_available_semaphore = device->create_semaphore(); 
	auto render_finished_semaphore = device->create_semaphore(); 
	std::array render_wait_semaphores = { image_available_semaphore.get_value() };
	std::array present_wait_semaphores = { render_finished_semaphore.get_value() };
	std::array render_finished_semaphores = { render_finished_semaphore.get_value() };

	auto pipeline_layout = device->create_pipeline_layout(PipelineLayoutCreateInfo());
	
	while(!glfwWindowShouldClose(win.get_handle()))
	{
		glfwPollEvents();

		device->acquire_swapchain_texture(swapchain.get_value(), image_available_semaphore.get_value());
		device->new_frame();

		auto list = device->allocate_cmd_list(QueueType::Gfx);

		std::array clear_values = { ClearValue(ClearColorValue({1, 1, 0, 1})) };
		std::array color_attachments = { device->get_swapchain_backbuffer_view(swapchain.get_value()) };
		
		RenderPassInfo info;
		info.render_area = Rect2D(0, 0, 1280, 720);
		info.color_attachments = color_attachments;
		info.clear_attachment_flags = 1 << 0;
		info.store_attachment_flags = 1 << 0;
		info.clear_values = clear_values;
		
		std::array color_attachments_refs = { 0Ui32 };
		std::array subpasses = { RenderPassInfo::Subpass(color_attachments_refs,
			{},
			{},
			RenderPassInfo::DepthStencilMode::ReadWrite) };
		info.subpasses = subpasses;
		device->cmd_begin_render_pass(list, info);

		PipelineRenderPassState rp_state;

		std::array blends = { PipelineColorBlendAttachmentState() };
		std::array shaders = {
			PipelineShaderStage(gfx::ShaderStageFlagBits::Vertex, 
			Device::get_backend_shader(vert_shader.get_value()), 
			"main" ),
			PipelineShaderStage(gfx::ShaderStageFlagBits::Fragment, 
			Device::get_backend_shader(frag_shader.get_value()), 
			"main" ),
		};

		rp_state.color_blend.attachments = blends;
		
		PipelineMaterialState mat_state;
		mat_state.stages = shaders;
		
		device->cmd_set_render_pass_state(list, rp_state);
		device->cmd_set_material_state(list, mat_state);
		device->cmd_bind_pipeline_layout(list, pipeline_layout.get_value());
		device->cmd_draw(list, 3, 1, 0, 0);
		device->cmd_end_render_pass(list);
		
		device->submit(list, render_wait_semaphores, render_finished_semaphores);
		device->end_frame();
		
		device->present(swapchain.get_value(), present_wait_semaphores);
	}

	
	glfwTerminate();
	
	return 0;
}