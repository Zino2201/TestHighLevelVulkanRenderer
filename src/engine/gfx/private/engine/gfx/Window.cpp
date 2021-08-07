#include "engine/gfx/Window.hpp"
#include <GLFW/glfw3.h>
#if CB_PLATFORM(WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

namespace cb
{

void on_window_size_changed(GLFWwindow* glfw_window, int width, int height)
{
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->width = width;
	window->height = height;
	window->window_resized_delegate.call(static_cast<uint32_t>(width),
		static_cast<uint32_t>(height));
}

Window::Window(const uint32_t in_width,
	const uint32_t in_height,
	WindowFlags in_flags) : window(nullptr), width(in_width), height(in_height), flags(std::move(in_flags))
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, 
		height, 
		"CityBuilder", 
		nullptr, 
		nullptr);

	glfwSetWindowUserPointer(window, this);

	/** Binds callbacks */
	glfwSetWindowSizeCallback(window, &on_window_size_changed);

	if(flags & WindowFlagBits::Centered)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);

		const uint32_t x = (static_cast<uint32_t>(video_mode->width) / 2) - (width / 2);
		const uint32_t y = (static_cast<uint32_t>(video_mode->height) / 2) - (height / 2);
		glfwSetWindowPos(window, x, y);
	}
}

Window::~Window()
{
	if(window)
		glfwDestroyWindow(window);
	window = nullptr;
}

void* Window::get_native_handle() const
{
#if CB_PLATFORM(WINDOWS)
	return glfwGetWin32Window(window);
#else
	CB_ASSERTF(false, "Unsupported platform");
#endif
}

	
}