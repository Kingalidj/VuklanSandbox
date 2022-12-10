#include "application.h"
#include "renderer.h"

#include "vk_engine.h"
#include "window.h"

#include "optick.h"

#include <imgui.h>
#include <implot.h>

namespace Atlas {

	Application *Application::s_Instance = nullptr;

	Application::Application()
	{
		CORE_ASSERT(!s_Instance, "Application already created!");
		s_Instance = this;

		m_ViewportSize = { 1600, 900 };

		WindowInfo info = { "Vulkan Engine", 1600, 900 };
		m_Window = make_scope<Window>(info);
		m_Window->set_event_callback(BIND_EVENT_FN(Application::on_event));

		m_Engine = make_scope<vkutil::VulkanEngine>(*m_Window.get());

		m_ImGuiLayer = make_ref<ImGuiLayer>();
		m_ImGuiLayer->on_attach();

		m_ColorTexture = Texture((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y, TextureFormat::R8G8B8A8);
		m_DepthTexture = Texture((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y, TextureFormat::D32);

		Render2D::init();
	}

	Application::~Application()
	{
		m_Engine->wait_idle();

		Render2D::cleanup();

		for (uint32_t i = 0; i < m_LayerStack.size(); i++) {
			Ref<Layer> layer = m_LayerStack.back();
			layer->on_detach();
			m_LayerStack.pop_back();
		}

		m_Engine->cleanup();
		m_Window->destroy();

	}

	void Application::run()
	{

		std::vector<float> data;
		std::pair<float, uint32_t> averageFrameTime = { 0.0f, 0 };
		float frameTime = 0;

		const uint32_t averageFrameCount = 10;
		const uint32_t metricsSize = 100;

		m_LastFrameTime = (float)glfwGetTime();

		while (!m_Window->should_close()) {

			ATL_FRAME("MainThread");

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			averageFrameTime.first += timestep;
			averageFrameTime.second++;

			if (averageFrameTime.second == averageFrameCount) {
				frameTime = averageFrameTime.first / averageFrameCount;
				data.push_back(1 / frameTime);
				averageFrameTime = { 0.0f, 0 };

				if (data.size() >= metricsSize)
					data.erase(data.begin());
			}

			//data.push_back(1 / timestep);

			if (!m_WindowMinimized) {

				ATL_EVENT("draw loop");

				m_ImGuiLayer->begin();
				uint32_t swapchainImageIndex;
				m_Engine->prepare_frame(&swapchainImageIndex);

				{
					Render2D::begin(m_ColorTexture, m_DepthTexture, { 255 });
					ATL_EVENT("layer::on_update");
					for (auto &layer : m_LayerStack) {
						layer->on_update(timestep);
					}
					Render2D::end();
				}

				for (auto &layer : m_LayerStack) layer->on_imgui();

				//ImPlot::PlotLine("framerate", m_FrameRates.data(), nullptr, 100);
				if (ImGui::Begin("Metrics")) {
					ImPlot::SetNextAxesToFit();
					ImPlot::BeginPlot("Framerate");
					ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit);
					ImPlot::SetupAxis(ImAxis_Y1, "FPS");
					ImPlot::PlotLine("", data.data(), (int)data.size());
					ImPlot::EndPlot();

					ImGui::Text("average: %.3f ms/frame (%.1f FPS)", frameTime * 1000, 1 / frameTime);
				}
				ImGui::End();

				render_viewport();

				m_Engine->exec_swapchain_renderpass(swapchainImageIndex, { 0, 0, 0, 0 }, [&]() {
					m_ImGuiLayer->on_imgui();
					});

				m_Engine->end_frame(swapchainImageIndex);
				m_ImGuiLayer->end();
			}

			m_Window->on_update();

			for (Event e : m_QueuedEvents) on_event(e);
			m_QueuedEvents.clear();
		}
	}

	void Application::render_viewport()
	{
		ATL_EVENT();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar();

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		glm::vec2 viewportBounds[2]{};
		viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };
		auto viewportSize = viewportBounds[1] - viewportBounds[0];

		ImGui::Image(m_ColorTexture.get_id(), { viewportSize.x, viewportSize.y });
		ImGui::End();

		ImGui::ShowDemoWindow();

		if (viewportSize.x != m_ViewportSize.x || viewportSize.y != m_ViewportSize.y) {
			Atlas::ViewportResizedEvent event = { (uint32_t)viewportSize.x, (uint32_t)viewportSize.y };
			Atlas::Event e(event);
			queue_event(e);
		}
	}

	//vkutil::VulkanManager &Application::get_vulkan_manager()
	//{
	//	return get_instance()->m_Engine->get_manager();
	//}

	vkutil::VulkanEngine &Application::get_engine()
	{
		return *get_instance()->m_Engine.get();
	}

	Window &Application::get_window()
	{
		return *get_instance()->m_Window.get();
	}

	Application *Application::get_instance()
	{
		CORE_ASSERT(s_Instance, "Application is not initialized!");
		return s_Instance;
	}

	glm::vec2 Application::get_mouse()
	{
		return get_instance()->m_Window->get_mouse_pos();
	}

	bool Application::is_key_pressed(KeyCode key)
	{
		return get_instance()->m_Window->is_key_pressed(key);
	}

	bool Application::is_mouse_pressed(int button)
	{
		return get_instance()->m_Window->is_mouse_button_pressed(button);
	}

	void Application::push_layer(Ref<Layer> layer)
	{
		m_LayerStack.push_back(layer);
		layer->on_attach();
	}

	void Application::queue_event(Event event)
	{
		m_QueuedEvents.push_back(event);
	}

	glm::vec2 &Application::get_viewport_size()
	{
		return get_instance()->m_ViewportSize;
	}

	Texture &Application::get_viewport_color_texture()
	{
		return get_instance()->m_ColorTexture;
	}

	Texture &Application::get_viewport_depth_texture()
	{
		return get_instance()->m_DepthTexture;
	}

	void Application::on_event(Event &event)
	{
		//if (!event.in_category(EventCategoryMouse)) CORE_TRACE("event: {}", event);

		EventDispatcher(event)
			.dispatch<WindowResizedEvent>(BIND_EVENT_FN(Application::on_window_resized))
			.dispatch<ViewportResizedEvent>(BIND_EVENT_FN(Application::on_viewport_resized));

		for (auto &layer : m_LayerStack) {
			if (event.handled) break;

			layer->on_event(event);
		}
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
		m_ViewportSize = { e.width, e.height };

		m_ColorTexture = Texture(e.width, e.height, TextureFormat::R8G8B8A8);
		m_DepthTexture = Texture(e.width, e.height, TextureFormat::D32);

		return false;
	}
}