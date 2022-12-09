# == vk-bootstrap ==

add_subdirectory(vk-bootstrap)

# == vma ==

add_subdirectory(VulkanMemoryAllocator)
include_directories(VulkanMemoryAllocator/include)

# == glfw ==

add_subdirectory(glfw)
include_directories(glfw/include)

# == spdlog ==

add_subdirectory(spdlog)
include_directories(spdlog/include)

# == glm ==

add_subdirectory(glm)
include_directories(glm)

# == stb ==

add_subdirectory(stb)

# == tinyobjloader ==

add_subdirectory(tinyobjloader)
include_directories(tinyobjloader)

# == imgui ==

add_library(imgui STATIC)

target_sources( imgui
	PRIVATE
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui.cpp

	imgui/backends/imgui_impl_vulkan.cpp
	imgui/backends/imgui_impl_glfw.cpp
	)

target_include_directories( imgui
	PUBLIC imgui
	PUBLIC imgui/backends

	PUBLIC ${VULKAN_INCLUDE_DIRS}
	)

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# == implot ==

add_library(implot STATIC)

target_sources( implot
	PRIVATE
	implot/implot.cpp
	implot/implot_demo.cpp
	implot/implot_items.cpp
	)

target_include_directories( implot
		PUBLIC implot
	)

target_link_libraries(implot PUBLIC imgui)

# == optick ==

add_subdirectory(optick)
include_directories(optick/include)
