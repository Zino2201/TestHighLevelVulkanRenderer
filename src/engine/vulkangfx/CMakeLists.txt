find_package(Vulkan REQUIRED)

cb_add_module(vulkangfx 
	public/engine/gfx/VulkanBackend.hpp
	private/engine/gfx/Vulkan.hpp
	private/engine/gfx/VulkanDevice.hpp
	private/engine/gfx/VulkanSwapChain.hpp
	private/engine/gfx/VulkanShader.hpp
	private/engine/gfx/VulkanBuffer.hpp
	private/engine/gfx/VulkanPipeline.hpp
	private/engine/gfx/VulkanTexture.hpp
	private/engine/gfx/VulkanPipelineLayout.cpp
	private/engine/gfx/VulkanPipelineLayout.hpp
	private/engine/gfx/VulkanRenderPass.hpp
	private/engine/gfx/VulkanCommandPool.hpp
	private/engine/gfx/VulkanCommandList.hpp
	private/engine/gfx/VulkanTextureView.hpp
	private/engine/gfx/VulkanSync.hpp
	private/engine/gfx/VulkanDevice.cpp
	private/engine/gfx/VulkanBackend.cpp
	private/engine/gfx/VulkanBuffer.cpp
	private/engine/gfx/VulkanSwapChain.cpp)
target_include_directories(vulkangfx PUBLIC public PRIVATE private)
target_link_libraries(vulkangfx PUBLIC core gfx 
	PRIVATE Vulkan::Vulkan vk-bootstrap::vk-bootstrap robin_hood::robin_hood VulkanMemoryAllocator)
target_compile_definitions(vulkangfx PRIVATE CB_VULKAN_GFX_PRIVATE)