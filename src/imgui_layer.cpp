#include "imgui_layer.h"
#include "window.h"
#include "application.h"
#include "vk_engine.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <implot.h>

#include <glm/glm.hpp>

#include "imgui_theme.h"

namespace Atlas {

	void ImGuiLayer::on_attach()
	{
		ImPlot::CreateContext();

		ImGui::SetOneDarkTheme();
	}

	void ImGuiLayer::on_detach()
	{
		ImPlot::DestroyContext();
	}

	void ImGuiLayer::on_update(Timestep ts)
	{
	}

	void ImGuiLayer::on_imgui()
	{
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Application::get_engine().get_active_command_buffer());
	}

	void ImGuiLayer::begin()
	{
		ATL_EVENT();
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
	}

	void ImGuiLayer::end()
	{
		ImGuiIO &io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow *backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

}
