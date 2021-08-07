#pragma once

#include "engine/Core.hpp"
#include "engine/Flags.hpp"
#include "engine/MulticastDelegate.hpp"

struct GLFWwindow;

namespace cb
{

enum class WindowFlagBits
{
	/** Center the window on the primary monitor */
	Centered = 1 << 0	
};
CB_ENABLE_FLAG_ENUMS(WindowFlagBits, WindowFlags);
	
/**
 * A OS window that can be drawn into
 */
class Window final
{
	friend void on_window_size_changed(GLFWwindow*, int, int);

public:
	Window(const uint32_t in_width,
		const uint32_t in_height,
		WindowFlags in_flags = WindowFlags());
	~Window();

	Window(const Window&) = delete;
	Window(Window&&) noexcept = default;
	
	void operator=(const Window&) = delete;
	Window& operator=(Window&&) noexcept = default;

	[[nodiscard]] GLFWwindow* get_handle() const { return window; }
	[[nodiscard]] void* get_native_handle() const;
	[[nodiscard]] auto& get_window_resized() { return window_resized_delegate; }
	[[nodiscard]] uint32_t get_width() const { return width; }
	[[nodiscard]] uint32_t get_height() const { return height; }
private:
	GLFWwindow* window;
	uint32_t width;
	uint32_t height;
	WindowFlags flags;
	MulticastDelegate<uint32_t, uint32_t> window_resized_delegate;
};

	
}