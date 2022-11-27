#pragma once
#include "event.h"

#include "layer.h"
#include "imgui_layer.h"
#include "texture.h"

#include <glm/glm.hpp>

class Window;

namespace vkutil {
	class VulkanEngine;
}

namespace Atlas {


	class Application {
	public:

		Application();
		~Application();

		void run();

		//static vkutil::VulkanManager &get_vulkan_manager();
		static vkutil::VulkanEngine &get_engine();
		static Window &get_window();
		static Application *get_instance();

		void push_layer(Ref<Layer> layer);

		void queue_event(Event event);

	private:
		void on_event(Event &event);
		bool on_window_resized(WindowResizedEvent &e);
		bool on_viewport_resized(ViewportResizedEvent &e);

		void render_viewport();

		void dyn_renderpass(Texture &color, Texture &depth, glm::vec4 clearColor, std::function<void()> &&func);

		Scope<vkutil::VulkanEngine> m_Engine;

		Scope<Window> m_Window;
		bool m_WindowMinimized = false;

		float m_LastFrameTime;

		Ref<ImGuiLayer> m_ImGuiLayer;
		std::vector<Ref<Layer>> m_LayerStack;

		std::vector<Event> m_QueuedEvents;

		Texture *m_ColorTexture;
		Texture *m_DepthTexture;
		glm::ivec2 m_ViewportSize;

		static Application *s_Instance;
	};

}
