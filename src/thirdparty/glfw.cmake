set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# Build GLFW as a DLL so we can properly use it across modules/DLL boundaries
set(GLFW_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
	GIT_REPOSITORY https://github.com/Zino2201/glfw.git)
FetchContent_MakeAvailable(glfw)
set_target_properties(glfw PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CB_BINS_DIR}")