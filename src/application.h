#pragma once
#include "event.h"

class Window;

namespace vkutil {
	class VulkanEngine;
}

namespace Atlas {

	class Timestep {
	public:

		Timestep(float time = 0.0f)
			: m_Time(time) {}

		operator float() const { return m_Time; }
		operator double() const { return m_Time; }

		inline float get_seconds() const { return m_Time; }
		inline float get_milliseconds() const { return m_Time; }

	private:
		float m_Time;
	};

	class Layer {
	public:

		virtual ~Layer() {};

		virtual void on_attach() {};
		virtual void on_detach() {};
		virtual void on_update(Timestep ts) {};
		virtual void on_imgui() {};
		virtual void on_event(Event &event) {};
	};

	class Application {
	public:

		Application();
		~Application();

		void run();


	private:
		void on_event(Event &event);
		bool on_window_resized(WindowResizedEvent &e);
		bool on_viewport_resized(ViewportResizedEvent &e);

		Scope<vkutil::VulkanEngine> m_Engine = nullptr;

		Scope<Window> m_Window = nullptr;
		bool m_WindowMinimized = false;

	};

}
