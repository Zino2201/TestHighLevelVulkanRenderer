cb_add_module(imgui
	private/engine/imgui/ImGui.cpp
	public/engine/imgui/ImGui.hpp
	${IMGUI_ROOT_DIR}/imgui.cpp
	${IMGUI_ROOT_DIR}/imgui_draw.cpp
	${IMGUI_ROOT_DIR}/imgui_tables.cpp
	${IMGUI_ROOT_DIR}/imgui_widgets.cpp
	${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp)
target_include_directories(imgui PUBLIC public ${IMGUI_ROOT_DIR} PRIVATE private ${DXC_INCLUDE_DIR})
target_link_directories(imgui PRIVATE ${DXC_LIB_DIR})
target_link_libraries(imgui PUBLIC core gfx PRIVATE dxcompiler glfw)