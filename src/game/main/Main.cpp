#include "engine/gfx/Window.hpp"
#include <GLFW/glfw3.h>
#include "engine/gfx/VulkanBackend.hpp"
#include "engine/gfx/BackendDevice.hpp"
#include "engine/gfx/Device.hpp"
#include "engine/logger/Logger.hpp"
#include <fstream>
#include "engine/logger/sinks/StdoutSink.hpp"
#if CB_PLATFORM(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <glm/gtc/matrix_transform.hpp>

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

#include <glm/glm.hpp>

using namespace cb::gfx;
struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;

	static VertexInputBindingDescription get_binding_description()
	{
		return VertexInputBindingDescription(0, sizeof(Vertex), VertexInputRate::Vertex);		
	}

	static std::vector<VertexInputAttributeDescription> get_attribute_descriptions()
	{
		return
		{
			VertexInputAttributeDescription(0, 0, Format::R32G32Sfloat, offsetof(Vertex, position)),
			VertexInputAttributeDescription(1, 0, Format::R32G32B32Sfloat, offsetof(Vertex, color)),
		};
	}
};

struct UBO
{
	glm::mat4 wvp;
};

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

int main()
{
	using namespace cb;

#if CB_PLATFORM(WINDOWS)
	DWORD mode;
	GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

	logger::set_pattern("[{time}] [{severity}] ({category}) {message}");
	logger::add_sink(std::make_unique<logger::StdoutSink>());

	glfwInit();
	
	Window win(1280, 
		720, 
		WindowFlags(WindowFlagBits::Centered));

	auto result = create_vulkan_backend(gfx::BackendFlags(gfx::BackendFlagBits::DebugLayers));
	if(!result)
	{
		logger::fatal("Failed to create backend: {}", result.get_error());
		return -1;
	}

	auto backend_device = result.get_value()->create_device(gfx::ShaderModel::SM6_0);
	if(!backend_device)
	{
		logger::fatal("Failed to create device: {}", backend_device.get_error());
		return -1;
	}
	
	auto device = std::make_unique<gfx::Device>(*result.get_value().get(), std::move(backend_device.get_value()));

	using namespace gfx;
	
	UniqueSwapchain swapchain(device->create_swapchain(gfx::SwapChainCreateInfo(
		win.get_native_handle(),
		win.get_width(),
		win.get_height())).get_value());

	win.get_window_resized().bind([&](uint32_t width, uint32_t height)
	{
		UniqueSwapchain old_swapchain(swapchain.free());

		device->wait_idle();
		swapchain = UniqueSwapchain(device->create_swapchain(gfx::SwapChainCreateInfo(
			win.get_native_handle(),
			width,
			height,
			device->get_swapchain_backend_handle(old_swapchain.get()))).get_value());
	});

	auto vert_spv = read_binary_file("vert.spv");
	auto frag_spv = read_binary_file("frag.spv");

	UniqueShader vert_shader(device->create_shader(ShaderCreateInfo(
		{ (uint32_t*) vert_spv.data(), vert_spv.size() })).get_value());
	UniqueShader frag_shader(device->create_shader(ShaderCreateInfo(
		{ (uint32_t*) frag_spv.data(), frag_spv.size() })).get_value());

	UniqueSemaphore image_available_semaphore(device->create_semaphore().get_value()); 
	UniqueSemaphore render_finished_semaphore(device->create_semaphore().get_value()); 
	std::array render_wait_semaphores = { image_available_semaphore.get() };
	std::array present_wait_semaphores = { render_finished_semaphore.get() };
	std::array render_finished_semaphores = { render_finished_semaphore.get() };

	std::array<DescriptorSetLayoutCreateInfo, 1> layouts;
	std::vector<DescriptorSetLayoutBinding> bindings =
	{
		DescriptorSetLayoutBinding(0, DescriptorType::UniformBuffer, 1, ShaderStageFlags(ShaderStageFlagBits::Vertex))
	};

	layouts[0].bindings = bindings;
	UniquePipelineLayout pipeline_layout(device->create_pipeline_layout(PipelineLayoutCreateInfo(layouts)).get_value());

	/** Buffer */
	UniqueBuffer vertex_buffer(device->create_buffer(BufferInfo(BufferCreateInfo(
		sizeof(Vertex) * vertices.size(),
		MemoryUsage::GpuOnly,
		BufferUsageFlags(BufferUsageFlagBits::VertexBuffer)), 
		{ (uint8_t*) vertices.data(), vertices.size() * sizeof(Vertex) })).get_value());


	UniqueBuffer ubo(device->create_buffer(BufferInfo::make_ubo(sizeof(UBO))).get_value());

	while(!glfwWindowShouldClose(win.get_handle()))
	{
		glfwPollEvents();

		if(device->acquire_swapchain_texture(swapchain.get(), 
			image_available_semaphore.get()) != gfx::Result::Success)
			continue;

		device->new_frame();

		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		glm::mat4 model = glm::rotate(glm::mat4(1.f), delta_time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
		glm::mat4 view = glm::lookAtRH(glm::vec3(2.f, 2.f, 2.f),
		glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 0.f, 1.f));
		glm::mat4 proj = glm::perspective(glm::radians(90.f),
		(float) win.get_width() / win.get_height(),
		0.f,
		1000.f);
		proj[1][1] *= -1;
		
		UBO ubo_data { proj * view * model };

		auto ubo_map = device->map_buffer(ubo.get());
		memcpy(ubo_map.get_value(), &ubo_data, sizeof(ubo_data));
		device->unmap_buffer(ubo.get());

		auto list = device->allocate_cmd_list(QueueType::Gfx);

		std::array clear_values = { ClearValue(ClearColorValue({1, 1, 0, 1})) };
		std::array color_attachments = { device->get_swapchain_backbuffer_view(swapchain.get()) };
		
		RenderPassInfo info;
		info.render_area = Rect2D(0, 0, win.get_width(), win.get_height());
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
			Device::get_backend_shader(vert_shader.get()), 
			"main" ),
			PipelineShaderStage(gfx::ShaderStageFlagBits::Fragment, 
			Device::get_backend_shader(frag_shader.get()), 
			"main" ),
		};

		rp_state.color_blend.attachments = blends;
		
		PipelineMaterialState mat_state;
		mat_state.stages = shaders;
		mat_state.vertex_input.input_binding_descriptions = { Vertex::get_binding_description() };
		mat_state.vertex_input.input_attribute_descriptions = Vertex::get_attribute_descriptions();

		device->cmd_set_render_pass_state(list, rp_state);
		device->cmd_set_material_state(list, mat_state);
		device->cmd_bind_pipeline_layout(list, pipeline_layout.get());
		device->cmd_bind_vertex_buffer(list, vertex_buffer.get(), 0);
		device->cmd_bind_ubo(list, 0, 0, ubo.get());
		device->cmd_draw(list, 3, 1, 0, 0);
		device->cmd_end_render_pass(list);
		device->submit(list, render_wait_semaphores, render_finished_semaphores);
		device->end_frame();
		
		device->present(swapchain.get(), present_wait_semaphores);
	}

	
	glfwTerminate();
	
	return 0;
}