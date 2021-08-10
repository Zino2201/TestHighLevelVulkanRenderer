cb_add_module(gfx 
	public/engine/gfx/Window.hpp
	public/engine/gfx/Device.hpp
	public/engine/gfx/ShaderFormat.hpp
	public/engine/gfx/Backend.hpp
	public/engine/gfx/Result.hpp
	public/engine/gfx/Memory.hpp
	public/engine/gfx/Buffer.hpp
	public/engine/gfx/Texture.hpp
	public/engine/gfx/BackendDevice.hpp
	public/engine/gfx/Format.hpp
	public/engine/gfx/Shader.hpp
	public/engine/gfx/Pipeline.hpp
	public/engine/gfx/PipelineLayout.hpp
	public/engine/gfx/GfxPipeline.hpp
	public/engine/gfx/RenderPass.hpp
	public/engine/gfx/DeviceResource.hpp
	public/engine/gfx/Sampler.hpp
	public/engine/gfx/SwapChain.hpp
	public/engine/gfx/Command.hpp
	public/engine/gfx/Sync.hpp
	public/engine/gfx/Rect.hpp
	private/engine/gfx/Window.cpp
	private/engine/gfx/ThreadedCommandPool.cpp
	private/engine/gfx/Device.cpp)
target_include_directories(gfx PUBLIC public PRIVATE private)
target_link_libraries(gfx PUBLIC core PRIVATE glfw)