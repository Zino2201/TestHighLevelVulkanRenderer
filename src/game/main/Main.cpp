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
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

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
	glm::vec3 position;
	glm::vec2 texcoord;
	glm::vec3 normal;

	static VertexInputBindingDescription get_binding_description()
	{
		return VertexInputBindingDescription(0, sizeof(Vertex), VertexInputRate::Vertex);		
	}

	static std::vector<VertexInputAttributeDescription> get_attribute_descriptions()
	{
		return
		{
			VertexInputAttributeDescription(0, 0, Format::R32G32B32Sfloat, offsetof(Vertex, position)),
			VertexInputAttributeDescription(1, 0, Format::R32G32Sfloat, offsetof(Vertex, texcoord)),
			VertexInputAttributeDescription(2, 0, Format::R32G32B32Sfloat, offsetof(Vertex, normal)),
		};
	}
};

struct UBO
{
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 proj;
};

int main()
{
	using namespace cb;

	logger::set_pattern("[{time}] [{severity}] ({category}) {message}");
	logger::add_sink(std::make_unique<logger::StdoutSink>());

	glfwInit();
	
	Window win(1280, 
		720, 
		WindowFlags(WindowFlagBits::Centered));

#if CB_BUILD(DEBUG)
	auto result = create_vulkan_backend(gfx::BackendFlags(gfx::BackendFlagBits::DebugLayers));
#else
	auto result = create_vulkan_backend(gfx::BackendFlags());
#endif
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


	auto vert_spv = read_binary_file("vert.spv");
	auto frag_spv = read_binary_file("frag.spv");

	UniqueShader vert_shader(device->create_shader(ShaderCreateInfo(
		{ (uint32_t*) vert_spv.data(), vert_spv.size() })).get_value());
	UniqueShader frag_shader(device->create_shader(ShaderCreateInfo(
		{ (uint32_t*) frag_spv.data(), frag_spv.size() })).get_value());

	UniqueSemaphore image_available_semaphore(device->create_semaphore(SemaphoreInfo()).get_value()); 
	UniqueSemaphore render_finished_semaphore(device->create_semaphore(SemaphoreInfo()).get_value()); 
	std::array render_wait_semaphores = { image_available_semaphore.get() };
	std::array present_wait_semaphores = { render_finished_semaphore.get() };
	std::array render_finished_semaphores = { render_finished_semaphore.get() };

	std::array<DescriptorSetLayoutCreateInfo, 1> layouts;
	std::vector<DescriptorSetLayoutBinding> bindings =
	{
		DescriptorSetLayoutBinding(0, DescriptorType::UniformBuffer, 1, ShaderStageFlags(ShaderStageFlagBits::Vertex)),
		DescriptorSetLayoutBinding(1, DescriptorType::Sampler, 1, ShaderStageFlags(ShaderStageFlagBits::Fragment)),
		DescriptorSetLayoutBinding(2, DescriptorType::SampledTexture, 1, ShaderStageFlags(ShaderStageFlagBits::Fragment)),
		DescriptorSetLayoutBinding(3, DescriptorType::SampledTexture, 1, ShaderStageFlags(ShaderStageFlagBits::Fragment)),
	};

	layouts[0].bindings = bindings;
	UniquePipelineLayout pipeline_layout(device->create_pipeline_layout(PipelineLayoutCreateInfo(layouts)).get_value());

	/** Buffer */

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "cube.obj");

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (const auto& shape : shapes) 
	{
		for (const auto& index : shape.mesh.indices) 
		{
			Vertex vertex;
			vertex.position = {
			    attrib.vertices[3 * index.vertex_index + 0],
			    attrib.vertices[3 * index.vertex_index + 1],
			    attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texcoord = {
			    attrib.texcoords[2 * index.texcoord_index + 0],
			    1.f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.normal = {
			    attrib.normals[3 * index.normal_index + 0],
			    attrib.normals[3 * index.normal_index + 1],
			    attrib.normals[3 * index.normal_index + 2]
			};


		    vertices.push_back(vertex);
			indices.push_back(indices.size());
		}
	}

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "sky.obj");

	std::vector<Vertex> sky_vertices;
	std::vector<uint32_t> sky_indices;

	for (const auto& shape : shapes) 
	{
		for (const auto& index : shape.mesh.indices) 
		{
			Vertex vertex;
			vertex.position = {
			    attrib.vertices[3 * index.vertex_index + 0],
			    attrib.vertices[3 * index.vertex_index + 1],
			    attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texcoord = {
			    attrib.texcoords[2 * index.texcoord_index + 0],
			    1.f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.normal = {
			    attrib.normals[3 * index.normal_index + 0],
			    attrib.normals[3 * index.normal_index + 1],
			    attrib.normals[3 * index.normal_index + 2]
			};


		    sky_vertices.push_back(vertex);
			sky_indices.push_back(sky_indices.size());
		}
	}


	UniqueBuffer vertex_buffer(device->create_buffer(BufferInfo(BufferCreateInfo(
		sizeof(Vertex) * vertices.size(),
		MemoryUsage::GpuOnly,
		BufferUsageFlags(BufferUsageFlagBits::VertexBuffer)), 
		{ (uint8_t*) vertices.data(), vertices.size() * sizeof(Vertex) })).get_value());

	UniqueBuffer index_buffer(device->create_buffer(BufferInfo(BufferCreateInfo(
		sizeof(uint32_t) * indices.size(),
		MemoryUsage::GpuOnly,
		BufferUsageFlags(BufferUsageFlagBits::IndexBuffer)), 
		{ (uint8_t*) indices.data(), indices.size() * sizeof(uint32_t) })).get_value());

	UniqueBuffer sky_vertex_buffer(device->create_buffer(BufferInfo(BufferCreateInfo(
		sizeof(Vertex) * sky_vertices.size(),
		MemoryUsage::GpuOnly,
		BufferUsageFlags(BufferUsageFlagBits::VertexBuffer)), 
		{ (uint8_t*) sky_vertices.data(), sky_vertices.size() * sizeof(Vertex) })).get_value());

	UniqueBuffer sky_index_buffer(device->create_buffer(BufferInfo(BufferCreateInfo(
		sizeof(uint32_t) * sky_indices.size(),
		MemoryUsage::GpuOnly,
		BufferUsageFlags(BufferUsageFlagBits::IndexBuffer)), 
		{ (uint8_t*) sky_indices.data(), sky_indices.size() * sizeof(uint32_t) })).get_value());

	size_t index_count = indices.size();
	vertices.clear();
	indices.clear();

	UniqueSampler sampler(device->create_sampler(SamplerCreateInfo()).get_value());

	
	UniqueTexture texture;
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load("Basecolor_carrelage_mur_zino.png", &width, &height, &channels, STBI_rgb_alpha);
		texture = UniqueTexture(device->create_texture(TextureInfo::make_immutable_2d(width,
			height,
			Format::R8G8B8A8Unorm,
			1,
			TextureUsageFlags(TextureUsageFlagBits::Sampled),
			{ pixels, pixels + (width * height * 4) })).get_value());
		stbi_image_free(pixels);
	}
	UniqueTextureView texture_view(device->create_texture_view(TextureViewInfo::make_2d(texture.get(),
		Format::R8G8B8A8Unorm).set_debug_name("basecolor")).get_value());

	UniqueTexture normal_map;
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load("Normal_carrelage_mur_zino.png", &width, &height, &channels, STBI_rgb_alpha);
		normal_map = UniqueTexture(device->create_texture(TextureInfo::make_immutable_2d(width,
			height,
			Format::R8G8B8A8Unorm,
			1,
			TextureUsageFlags(TextureUsageFlagBits::Sampled),
			{ pixels, pixels + (width * height * 4) })).get_value());
		stbi_image_free(pixels);
	}

	UniqueTextureView normal_map_view(device->create_texture_view(TextureViewInfo::make_2d(normal_map.get(),
		Format::R8G8B8A8Unorm).set_debug_name("Normal Map View")).get_value());

	UniqueTexture sky_texture;
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load("parking_lot_2k.png", &width, &height, &channels, STBI_rgb_alpha);
		sky_texture = UniqueTexture(device->create_texture(TextureInfo::make_immutable_2d(width,
			height,
			Format::R8G8B8A8Unorm,
			1,
			TextureUsageFlags(TextureUsageFlagBits::Sampled),
			{ pixels, pixels + (width * height * 4) })).get_value());
		stbi_image_free(pixels);
	}
	UniqueTextureView sky_texture_view(device->create_texture_view(TextureViewInfo::make_2d(sky_texture.get(),
		Format::R8G8B8A8Unorm).set_debug_name("Sky Texture View")).get_value());

	UniqueTexture depth_texture(device->create_texture(TextureInfo::make_depth_stencil_attachment(
		win.get_width(), win.get_height(), Format::D24UnormS8Uint).set_debug_name("Depth Buffer Texture")).get_value());

	UniqueTextureView depth_texture_view(device->create_texture_view(TextureViewInfo::make_depth(depth_texture.get(),
		Format::D24UnormS8Uint).set_debug_name("Depth Buffer View")).get_value());

	win.get_window_resized().bind([&](uint32_t width, uint32_t height)
	{
		logger::verbose("Resizing swapchain and recreating resources...");

		UniqueSwapchain old_swapchain(swapchain.free());

		device->wait_idle();
		swapchain = UniqueSwapchain(device->create_swapchain(gfx::SwapChainCreateInfo(
			win.get_native_handle(),
			width,
			height,
			device->get_swapchain_backend_handle(old_swapchain.get()))).get_value());

		depth_texture = UniqueTexture(device->create_texture(TextureInfo::make_depth_stencil_attachment(
			width,height, Format::D24UnormS8Uint).set_debug_name("Depth Buffer Texture")).get_value());

		depth_texture_view = UniqueTextureView(device->create_texture_view(TextureViewInfo::make_depth(depth_texture.get(),
			Format::D24UnormS8Uint).set_debug_name("Depth Buffer View")).get_value());
	});


	std::vector<UniqueBuffer> ubos;

	int sylvains = 1;

	for(size_t i = 0; i < sylvains; ++i)
		ubos.emplace_back(device->create_buffer(BufferInfo::make_ubo(sizeof(UBO))).get_value());

	UniqueBuffer ubo_sky(device->create_buffer(BufferInfo::make_ubo(sizeof(UBO))).get_value());

	float cam_pitch = 0.f, cam_yaw = 0.f;
	glfwSetInputMode(win.get_handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
	

		double xpos = 0.f, ypos = 0.f;
		static double last_xpos = 0.f;
		static double last_ypos = 0.f;
		glfwGetCursorPos(win.get_handle(), &xpos, &ypos);

		float delta_x = ypos - last_ypos;
		float delta_y = xpos - last_xpos;

		if(glfwGetInputMode(win.get_handle(),GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		{
			cam_yaw += -delta_y * 0.025f * delta_time;
			cam_pitch -= -delta_x * 0.025f * delta_time;

			if(cam_pitch > 89.f)
				cam_pitch = 89.f;

			if(cam_pitch < -89.f)
				cam_pitch = -89.f;
		}

		glm::vec3 fwd;
		static glm::vec3 cam_pos = glm::vec3(0, 0, 2);

		fwd.x = glm::cos(glm::radians(cam_yaw)) * glm::cos(glm::radians(cam_pitch));
		fwd.y = glm::sin(glm::radians(cam_yaw)) * glm::cos(glm::radians(cam_pitch));
		fwd.z = glm::sin(glm::radians(cam_pitch));
		fwd = glm::normalize(fwd);

		glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 0, 1)));
		if (glfwGetKey(win.get_handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetInputMode(win.get_handle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		if(glfwGetMouseButton(win.get_handle(), GLFW_MOUSE_BUTTON_LEFT))
			glfwSetInputMode(win.get_handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		float cam_speed = 0.0015f;
		if (glfwGetKey(win.get_handle(), GLFW_KEY_W) == GLFW_PRESS)
		    cam_pos -= fwd * cam_speed * delta_time;
		if (glfwGetKey(win.get_handle(), GLFW_KEY_S) == GLFW_PRESS)
		    cam_pos += fwd * cam_speed * delta_time;
		if (glfwGetKey(win.get_handle(), GLFW_KEY_A) == GLFW_PRESS)
		    cam_pos += right * cam_speed * delta_time;
		if (glfwGetKey(win.get_handle(), GLFW_KEY_D) == GLFW_PRESS)
		    cam_pos -= right * cam_speed * delta_time;


		last_xpos = xpos;
		last_ypos = ypos;


		glm::mat4 view = glm::lookAtLH(cam_pos,
			cam_pos + fwd,
			glm::vec3(0.f, 0.f, 1.f));
			glm::mat4 proj = glm::perspective(glm::radians(90.f),
			(float) win.get_width() / win.get_height(),
			0.01f,
			10000.f);
			proj[1][1] *= -1;
		
{
		glm::mat4 model = glm::scale(glm::mat4(1.f), glm::vec3(20.f, 20.f, 20.f)) 
			* glm::rotate(glm::mat4(1.f), delta_time * 0.05f * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
			UBO ubo_data;
			ubo_data.world = model;
			ubo_data.view = view;
			ubo_data.proj = proj;

			auto ubo_map = device->map_buffer(ubo_sky.get());
			memcpy(ubo_map.get_value(), &ubo_data, sizeof(ubo_data));
			device->unmap_buffer(ubo_sky.get());
		}
		for(size_t i = 0; i < sylvains; ++i)
		{
			glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(i * -0.5f, 0, 0));
		
			UBO ubo_data;
			ubo_data.world = model;
			ubo_data.view = view;
			ubo_data.proj = proj;
			//Ss
			auto ubo_map = device->map_buffer(ubos[i].get());
			memcpy(ubo_map.get_value(), &ubo_data, sizeof(ubo_data));
			device->unmap_buffer(ubos[i].get());
		}

		auto list = device->allocate_cmd_list(QueueType::Gfx);

		std::array clear_values = { ClearValue(ClearColorValue({0, 0, 0, 1})),
			ClearValue(ClearDepthStencilValue(1.f, 0))};
		std::array color_attachments = { device->get_swapchain_backbuffer_view(swapchain.get()) };
		
		RenderPassInfo info;
		info.render_area = Rect2D(0, 0, win.get_width(), win.get_height());
		info.color_attachments = color_attachments;
		info.clear_attachment_flags = 1 << 0;
		info.store_attachment_flags = 1 << 0;
		info.clear_values = clear_values;
		info.depth_stencil_attachment = depth_texture_view.get();
		
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
		rp_state.depth_stencil.enable_depth_test = true;
		rp_state.depth_stencil.enable_depth_write = true;
		rp_state.depth_stencil.enable_stencil_test = false;
		rp_state.depth_stencil.depth_compare_op = CompareOp::Less;

		PipelineMaterialState mat_state;
		mat_state.stages = shaders;
		mat_state.vertex_input.input_binding_descriptions = { Vertex::get_binding_description() };
		mat_state.vertex_input.input_attribute_descriptions = Vertex::get_attribute_descriptions();
		mat_state.rasterizer.cull_mode = CullMode::Back;
		mat_state.rasterizer.front_face = FrontFace::CounterClockwise;
		mat_state.rasterizer.polygon_mode = PolygonMode::Fill;

		device->cmd_set_render_pass_state(list, rp_state);
		device->cmd_set_material_state(list, mat_state);
		device->cmd_bind_pipeline_layout(list, pipeline_layout.get());

		device->cmd_bind_vertex_buffer(list, sky_vertex_buffer.get(), 0);
		device->cmd_bind_index_buffer(list, sky_index_buffer.get(), 0, IndexType::Uint32);
		device->cmd_bind_sampler(list, 0, 1, sampler.get());
		device->cmd_bind_texture_view(list, 0, 2, sky_texture_view.get());
		device->cmd_bind_texture_view(list, 0, 3, texture_view.get());
		device->cmd_bind_ubo(list, 0, 0, ubo_sky.get());
		device->cmd_draw_indexed(list, sky_indices.size(), 1, 0, 0, 0);

		device->cmd_bind_vertex_buffer(list, vertex_buffer.get(), 0);
		device->cmd_bind_index_buffer(list, index_buffer.get(), 0, IndexType::Uint32);

		device->cmd_bind_sampler(list, 0, 1, sampler.get());
		device->cmd_bind_texture_view(list, 0, 2, texture_view.get());
		device->cmd_bind_texture_view(list, 0, 3, normal_map_view.get());

		for(size_t i = 0; i < sylvains; ++i)
		{
			device->cmd_bind_ubo(list, 0, 0, ubos[i].get());
			device->cmd_draw_indexed(list, index_count, 1, 0, 0, 0);
		}
		device->cmd_end_render_pass(list);
		device->submit(list, render_wait_semaphores, render_finished_semaphores);
		device->end_frame();
		
		device->present(swapchain.get(), present_wait_semaphores);

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(6ms);
	}

	
	glfwTerminate();
	
	return 0;
}