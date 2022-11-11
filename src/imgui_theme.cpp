#include "imgui_theme.h"

void ImGui::SetOneDarkTheme() {
	auto ACCENT = RED_COL;
	auto ACCENT_HOVER = ORANGE_COL;

	ImGui::StyleColorsDark();

	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowBorderSize = 0;
	style.PopupBorderSize = 0;
	style.FramePadding = { 10, 5 };
	style.FrameRounding = 6;
	style.GrabRounding = 2;

	auto &colors = style.Colors;
	colors[ImGuiCol_WindowBg] = BLACK_COL;

	// Headers
	colors[ImGuiCol_Header] = DARK_GREY_COL;
	colors[ImGuiCol_HeaderHovered] = GREY_COL;
	colors[ImGuiCol_HeaderActive] = LIGHT_GREY_COL;
	colors[ImGuiCol_MenuBarBg] = BLACK_COL;

	// Buttons
	colors[ImGuiCol_Button] = DARK_GREY_COL;
	colors[ImGuiCol_ButtonHovered] = LIGHT_GREY_COL;
	colors[ImGuiCol_ButtonActive] = GREY_COL;

	// Frame BG
	colors[ImGuiCol_FrameBg] = DARK_GREY_COL;
	colors[ImGuiCol_FrameBgHovered] = LIGHT_GREY_COL;
	colors[ImGuiCol_FrameBgActive] = LIGHT_GREY_COL;

	// Seperator
	colors[ImGuiCol_SeparatorHovered] = ACCENT;
	colors[ImGuiCol_SeparatorActive] = ACCENT_HOVER;
	colors[ImGuiCol_ResizeGrip] = GREY_COL;
	colors[ImGuiCol_ResizeGripHovered] = ACCENT;
	colors[ImGuiCol_ResizeGripActive] = ACCENT_HOVER;

	// Plot
	colors[ImGuiCol_PlotHistogram] = ACCENT;
	colors[ImGuiCol_PlotHistogramHovered] = ACCENT_HOVER;


	// Docking
	colors[ImGuiCol_DockingPreview] = ACCENT;
	colors[ImGuiCol_DockingEmptyBg] = DARK_GREY_COL;


	// NavWindow
	colors[ImGuiCol_NavWindowingHighlight] = ACCENT;

	//Checkmarks
	colors[ImGuiCol_CheckMark] = ACCENT;
	colors[ImGuiCol_SliderGrab] = ACCENT;
	colors[ImGuiCol_SliderGrabActive] = ACCENT_HOVER;
	colors[ImGuiCol_DragDropTarget] = ACCENT;

	// Tabs
	colors[ImGuiCol_Tab] = LIGHT_GREY_COL;
	colors[ImGuiCol_TabHovered] = DARK_GREY_COL;
	colors[ImGuiCol_TabActive] = BLACK_COL;
	colors[ImGuiCol_TabUnfocused] = GREY_COL;
	colors[ImGuiCol_TabUnfocusedActive] = BLACK_COL;

	// Title 
	colors[ImGuiCol_TitleBg] = GREY_COL;
	colors[ImGuiCol_TitleBgActive] = LIGHT_GREY_COL;
	colors[ImGuiCol_TitleBgCollapsed] = DARK_GREY_COL;
}
