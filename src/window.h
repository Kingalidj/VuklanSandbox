#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "event.h"
#include "vk_types.h"


#include <string>
#include <functional>

struct GLFWwindow;

struct WindowInfo {
	std::string Title;
	unsigned int Width;
	unsigned int Height;

	WindowInfo(const std::string &title = "Application",
		uint32_t width = 1200,
		uint32_t height = 720)
		:Title(title), Width(width), Height(height) {
	}
};

using EventCallbackFn = std::function<void(Atlas::Event &)>;

class Window {
private:
	void init(const WindowInfo &info);

	GLFWwindow *m_Window;

	struct WindowData {
		std::string title;
		uint32_t width, height;

		EventCallbackFn eventCallback;
	};

	WindowData m_Data;

public:
	Window(const WindowInfo &props);
	void cleanup();

	void on_update();

	inline uint32_t get_width() const { return m_Data.width; }
	inline uint32_t get_height() const { return m_Data.height; }

	std::pair<float, float> get_mouse_pos() const;

	VkResult create_window_surface(VkInstance instance, VkSurfaceKHR *surface);

	bool should_close() const;

	inline void set_event_callback(const EventCallbackFn &callback) { m_Data.eventCallback = callback; }
	void capture_mouse(bool enabled) const;

	inline GLFWwindow *get_native_window() const { return m_Window; }

	const EventCallbackFn &get_event_callback() const { return m_Data.eventCallback; }
};
