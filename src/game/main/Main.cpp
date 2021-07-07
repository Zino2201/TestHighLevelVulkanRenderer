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

	
#if 0

	auto swap = device.get_value()->create_swap_chain(gfx::SwapChainCreateInfo(win.get_native_handle(), 
		1280, 
		720));

	auto buffer = device.get_value()->create_buffer(gfx::BufferCreateInfo(4, 
		gfx::MemoryUsage::CpuToGpu,
		gfx::BufferUsageFlags(gfx::BufferUsageFlagBits::VertexBuffer)));

	
	auto vert = device.get_value()->create_shader(
		gfx::ShaderCreateInfo({ (uint32_t*) vert_spv.data(), vert_spv.size() }));

	auto frag = device.get_value()->create_shader(
		gfx::ShaderCreateInfo({ (uint32_t*) frag_spv.data(), frag_spv.size() }));

	auto pool = device.get_value()->create_command_pool(gfx::CommandPoolCreateInfo { gfx::QueueType::Gfx });
	auto lists = device.get_value()->allocate_command_lists(
		pool.get_value(),
		1);

	auto list = lists.get_value()[0];

	auto rp = device.get_value()->create_render_pass(
		gfx::RenderPassCreateInfo(
		{
			gfx::AttachmentDescription(
				gfx::Format::B8G8R8A8Unorm,
				gfx::SampleCountFlagBits::Count1,
				gfx::AttachmentLoadOp::Clear,
				gfx::AttachmentStoreOp::Store,
				gfx::AttachmentLoadOp::DontCare,
				gfx::AttachmentStoreOp::DontCare,
				gfx::TextureLayout::Undefined,
				gfx::TextureLayout::Present)
		},
		{
			gfx::SubpassDescription(
				{},
				{ gfx::AttachmentReference(0, gfx::TextureLayout::ColorAttachment) },
				{},
				{},
				{})
		}));

	auto image_available_semaphore = device.get_value()->create_semaphore(gfx::SemaphoreCreateInfo());
	auto render_finished_semaphore = device.get_value()->create_semaphore(gfx::SemaphoreCreateInfo());
	auto fence = device.get_value()->create_fence(gfx::FenceCreateInfo(true));

	auto pipeline_layout = device.get_value()->create_pipeline_layout(gfx::PipelineLayoutCreateInfo());

	std::array color_states = {
		gfx::PipelineColorBlendAttachmentState()
	};
	auto pipeline = device.get_value()->create_gfx_pipeline(
		gfx::GfxPipelineCreateInfo(
{
			gfx::PipelineShaderStage(gfx::ShaderStageFlagBits::Vertex, vert.get_value(), "main" ),
			gfx::PipelineShaderStage(gfx::ShaderStageFlagBits::Fragment, frag.get_value(), "main" ),
			},
			gfx::PipelineVertexInputStateCreateInfo(),
			gfx::PipelineInputAssemblyStateCreateInfo(),
			gfx::PipelineRasterizationStateCreateInfo(),
			gfx::PipelineMultisamplingStateCreateInfo(),
			gfx::PipelineDepthStencilStateCreateInfo(),
			gfx::PipelineColorBlendStateCreateInfo(false, gfx::LogicOp::NoOp, color_states),
			pipeline_layout.get_value(),
			rp.get_value(),
			0));
#endif
	while(!glfwWindowShouldClose(win.get_handle()))
	{
		glfwPollEvents();

		device->new_frame();

		auto list = device->allocate_cmd_list(QueueType::Gfx);

		
#if 0
		device.get_value()->new_frame();

		std::array fences = { fence.get_value() };
		device.get_value()->wait_for_fences(fences);
		device.get_value()->reset_fences(fences);
		
		auto ra = device.get_value()->acquire_swapchain_image(swap.get_value(),
			image_available_semaphore.get_value());
		if(ra != gfx::Result::Success)
			continue;
		
		gfx::Framebuffer framebuffer;
		framebuffer.width = 1280;
		framebuffer.height = 720;
		framebuffer.layers = 1;
		std::array xd = { device.get_value()->get_swapchain_backbuffer_view(swap.get_value()) };
		framebuffer.attachments = xd;

		device.get_value()->reset_command_pool(pool.get_value());
		
		device.get_value()->begin_cmd_list(list);

		std::array caca = { gfx::ClearValue(gfx::ClearColorValue({ 0, 0, 0, 1 })) };
		device.get_value()->cmd_begin_render_pass(list, 
			rp.get_value(),
			framebuffer,
			gfx::Rect2D(0, 0, 1280, 720),
			caca);

		std::array viewports = { gfx::Viewport(0, 0, 1280, 720) };
		std::array scissors = { gfx::Rect2D(0, 0, 1280, 720) };
		
		device.get_value()->cmd_set_viewports(list, 0, viewports);
		device.get_value()->cmd_set_scissors(list, 0, scissors);
		device.get_value()->cmd_bind_pipeline(list,
			gfx::PipelineBindPoint::Gfx,
			pipeline.get_value());
		device.get_value()->cmd_draw(list,
			3,
			1,
			0,
			0);
		device.get_value()->cmd_end_render_pass(list);
		device.get_value()->end_cmd_list(list);

		std::array lists = { list };
		std::array wait_semaphore_submit = { image_available_semaphore.get_value() };
		std::array wait_semaphore_submit_flags = { gfx::PipelineStageFlags(gfx::PipelineStageFlagBits::TopOfPipe) };
		std::array signaled_semaphore = { render_finished_semaphore.get_value() };
		
		device.get_value()->queue_submit(gfx::QueueType::Gfx,
			lists,
			wait_semaphore_submit,
			wait_semaphore_submit_flags,
			signaled_semaphore,
			fence.get_value());
		
		std::array wait_semaphore = { render_finished_semaphore.get_value() };
		device.get_value()->present(swap.get_value(), wait_semaphore);
#endif
	}

	
	glfwTerminate();
	
	return 0;
}