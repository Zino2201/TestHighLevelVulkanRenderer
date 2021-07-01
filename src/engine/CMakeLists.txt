add_subdirectory(core)
add_subdirectory(gfx)

if(CB_WITH_VULKAN)
	add_subdirectory(vulkangfx)
endif()