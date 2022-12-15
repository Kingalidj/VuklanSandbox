cmake_minimum_required(VERSION 3.10)

project(Application)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE 1)

macro(print_all_variables)
	message(STATUS "print_all_variables------------------------------------------{")
	get_cmake_property(_variableNames VARIABLES)
	foreach (_variableName ${_variableNames})
		message(STATUS "${_variableName}=${${_variableName}}")
	endforeach()
	message(STATUS "print_all_variables------------------------------------------}")
endmacro()

set(SRC_DIR src)
set(RES_DIR res)

find_package(Vulkan 1.3 REQUIRED FATAL_ERROR)
add_subdirectory(vendor)

set(SOURCES
		src/render_api.cpp
		src/perspective_camera.cpp
		src/orthographic_camera.cpp
		src/descriptor.cpp
		src/buffer.cpp
		src/texture.cpp
		src/shader.cpp
		src/renderer.cpp
		src/application.cpp
		src/window.cpp
		src/imgui_layer.cpp
		src/imgui_theme.cpp
		src/atl_vk_utils.cpp
		src/logger.cpp

		src/vk_manager.cpp
		src/vk_types.cpp
		src/vk_descriptors.cpp
		src/vk_pipeline.cpp
		src/vk_engine.cpp
		src/vk_initializers.cpp
	)

set(HEADERS
		src/utils.h
		src/pch.h
		src/event.h
		src/layer.h

		src/render_api.cpp
		src/camera.cpp
		src/perspective_camera.h
		src/orthographic_camera.h
		src/atl_vk_utils.h
		src/descriptor.h
		src/buffer.h
		src/shader.h
		src/renderer.h
		src/imgui_layer.h
		src/texture.h
		src/application.h
		src/window.h
		src/imgui_theme.h
		src/logger.h

		src/vk_manager.h
		src/vk_types.h
		src/vk_descriptors.h
		src/vk_pipeline.h
		src/vk_engine.h
		src/vk_initializers.h
	)

set(EMBED
		src/robot_regular.embed
	)

add_executable(${PROJECT_NAME} src/main.cpp
	${SOURCES}
	${EMBED}
	${HEADERS}
	)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE ${SRC_DIR}/pch.h
	)

# target_compile_options(${PROJECT_NAME} PRIVATE "-Wall")

if (WIN32)
	get_filename_component(real_path "${RES_DIR}" REALPATH)
	string(REPLACE "/" "\\" target_path "${real_path}")

	add_custom_command( #link_resources ALL
		TARGET ${PROJECT_NAME}
		COMMAND cmd /C if not exist "${RES_DIR}" mklink /J ${RES_DIR} "${target_path}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		)

	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

else()
	add_custom_command(
		TARGET ${PROJECT_NAME}
		COMMAND 
		${CMAKE_COMMAND} -E create_symlink 
		${CMAKE_SOURCE_DIR}/${RES_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/${RES_DIR}
		)
endif()

target_include_directories(${PROJECT_NAME}
	PUBLIC ${Vulkan_INCLUDE_DIRS}
	)

get_filename_component(VULKAN_LIB ${Vulkan_LIBRARY} DIRECTORY)

find_library(SHADERC shaderc_combined
	HINTS ${VULKAN_LIB}
	)

find_library(SHADERC_D shaderc_combinedd
	HINTS ${VULKAN_LIB}
	)

find_library(SPIRV_CROSS spirv-cross-core
	HINTS ${VULKAN_LIB}
	)

find_library(SPIRV_CROSS_D spirv-cross-cored
	HINTS ${VULKAN_LIB}
	)

#find_library(SPIRV_CROSS_GLSL spirv-cross-glsl
#	HINTS ${VULKAN_LIB}
#	)
#
#find_library(SPIRV_CROSS_GLSL_D spirv-cross-glsld
#	HINTS ${VULKAN_LIB}
#	)

if (WIN32)

	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")

	target_link_libraries(${PROJECT_NAME} 
	PRIVATE
		$<$<CONFIG:Debug>:${SHADERC_D}>
		$<$<CONFIG:Debug>:${SPIRV_CROSS_D}>

		$<$<CONFIG:Release>:${SHADERC}>
		$<$<CONFIG:Release>:${SPIRV_CROSS}>

		$<$<CONFIG:RelWithDebInfo>:${SHADERC}>
		$<$<CONFIG:RelWithDebInfo>:${SPIRV_CROSS}>
		)

else()

	message(STATUS ${SPIRV_CROSS})

	target_link_libraries(${PROJECT_NAME} 
	PRIVATE
		${SHADERC}
		${SPIRV_CROSS}
		)
endif()

target_link_libraries(${PROJECT_NAME} 
PRIVATE
	Vulkan::Vulkan 
	glfw
	vk-bootstrap::vk-bootstrap
	VulkanMemoryAllocator
	spdlog
	glm
	stb_image
	tinyobjloader
	imgui
	implot
	OptickCore
	)

