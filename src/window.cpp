#include "window.h"

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char *message) {
	CORE_ERROR("GLFW Error ({0}): {1}", error, message);
}

Window::Window(const WindowInfo &props) {
	init(props);
}

using namespace Atlas;

bool Window::is_key_pressed(Atlas::KeyCode key)
{
	auto state = glfwGetKey(m_Window, (int)key);
	return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Window::is_mouse_button_pressed(int button)
{
	auto state = glfwGetMouseButton(m_Window, button);
	return state == GLFW_PRESS;
}

glm::vec2 Window::get_mouse_pos()
{
	double mouseX, mouseY;
	glfwGetCursorPos(m_Window, &mouseX, &mouseY);
	return { (float)mouseX, (float)mouseY };
}

void Window::init(const WindowInfo &props) {

	m_Data.title = props.Title;
	m_Data.width = props.Width;
	m_Data.height = props.Height;

	CORE_TRACE("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);


	if (!s_GLFWInitialized) {
		int success = glfwInit();
		CORE_ASSERT(success, "Could not intialize GLFW!");
		glfwSetErrorCallback(GLFWErrorCallback);

		s_GLFWInitialized = true;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.title.c_str(), nullptr, nullptr);

	{
		int w, h;
		glfwGetFramebufferSize(m_Window, &w, &h);
		m_Data.width = static_cast<uint32_t>(w);
		m_Data.height = static_cast<uint32_t>(h);
	}

	glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	glfwSetWindowUserPointer(m_Window, &m_Data);

	glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height)
		{
			WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
	data.width = width;
	data.height = height;

	WindowResizedEvent event{ (uint32_t)width, (uint32_t)height };
	Event e(event);
	data.eventCallback(e);
		});

	glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window)
		{
			WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
	WindowClosedEvent event;
	Event e(event);
	data.eventCallback(e);
		});


	glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
		{
			WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

	switch (action) {
	case GLFW_PRESS:
	{
		KeyPressedEvent event{ key, 0 };
		Event e(event);
		data.eventCallback(e);
		break;
	}
	case GLFW_RELEASE:
	{
		KeyReleasedEvent event{ key };
		Event e(event);
		data.eventCallback(e);
		break;
	}
	case GLFW_REPEAT:
	{
		KeyPressedEvent event{ key, 1 };
		Event e(event);
		data.eventCallback(e);
		break;
	}
	}
		});

	glfwSetCharCallback(m_Window, [](GLFWwindow *window, unsigned int keyCode)
		{
			WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
	KeyTypedEvent event{ (int)keyCode };
	Event e(event);
	data.eventCallback(e);
		});

	glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *window, int button, int action, int mods)
		{
			WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

	switch (action) {
	case GLFW_PRESS:
	{
		MouseButtonPressedEvent event{ button };
		Event e(event);
		data.eventCallback(e);
		break;
	}
	case GLFW_RELEASE:
	{
		MouseButtonReleasedEvent event{ button };
		Event e(event);
		data.eventCallback(e);
		break;
	}
	}
		});

	glfwSetScrollCallback(m_Window, [](GLFWwindow *window, double xOffset, double yOffset)
		{
			WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

	MouseScrolledEvent event{ (float)xOffset, (float)yOffset };
	Event e(event);
	data.eventCallback(e);
		});

	glfwSetCursorPosCallback(m_Window, [](GLFWwindow *window, double xPos, double yPos)
		{
			WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

	MouseMovedEvent event{ (float)xPos, (float)yPos };
	Event e(event);
	data.eventCallback(e);
		});

}

void Window::destroy() {

	glfwDestroyWindow(m_Window);
}

void Window::on_update() {

	glfwPollEvents();
}

std::pair<float, float> Window::get_mouse_pos() const {
	double mouseX, mouseY;
	glfwGetCursorPos(m_Window, &mouseX, &mouseY);
	return { (float)mouseX, (float)mouseY };
}

VkResult Window::create_window_surface(VkInstance instance, VkSurfaceKHR *surface) {
	return glfwCreateWindowSurface(instance, m_Window, nullptr, surface);
}

bool Window::should_close() const {
	return glfwWindowShouldClose(m_Window);
}

void Window::capture_mouse(bool enabled) const {
	if (enabled) {
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
