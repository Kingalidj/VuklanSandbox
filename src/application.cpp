#include "application.h"

#include "vk_engine.h"
#include "window.h"

namespace Atlas {

	Application::Application()
	{
		WindowInfo info = { "Vulkan Engine", 1600, 900 };
		m_Window = make_scope<Window>(info);
		m_Window->set_event_callback(BIND_EVENT_FN(Application::on_event));

		m_Engine = make_scope<vkutil::VulkanEngine>(*m_Window.get());
	}

	Application::~Application()
	{
		m_Engine->cleanup();
		m_Window->destroy();
	}

	void Application::run()
	{
		while (!m_Window->should_close()) {
			m_Window->on_update();

			if (!m_WindowMinimized) {
				m_Engine->draw();
			}
		}
	}


	void Application::on_event(Event &event)
	{
		EventDispatcher(event)
			.dispatch<WindowResizedEvent>(BIND_EVENT_FN(Application::on_window_resized))
			.dispatch<ViewportResizedEvent>(BIND_EVENT_FN(Application::on_viewport_resized));
	}

	bool Application::on_window_resized(WindowResizedEvent &e)
	{
		if (e.width == 0 || e.height == 0) m_WindowMinimized = true;
		else m_WindowMinimized = false;

		m_Engine->resize_window(e.width, e.height);

		return false;
	}
	bool Application::on_viewport_resized(ViewportResizedEvent &e)
	{
		m_Engine->resize_viewport(e.width, e.height);
		return false;
	}
}