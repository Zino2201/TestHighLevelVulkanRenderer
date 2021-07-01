#pragma once

#include "engine/Core.hpp"
#include "engine/Flags.hpp"

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
private:
	GLFWwindow* window;
	uint32_t width;
	uint32_t height;
	WindowFlags flags;
};

	
}