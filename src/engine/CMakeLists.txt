add_subdirectory(core)
add_subdirectory(gfx)
add_subdirectory(imgui)

if(CB_WITH_VULKAN)
	add_subdirectory(vulkangfx)
endif()