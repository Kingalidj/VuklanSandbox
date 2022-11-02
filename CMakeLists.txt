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


# add_compile_options( -g )

set(SRC_DIR src)
set(RES_DIR res)

find_package(Vulkan REQUIRED FATAL_ERROR)
add_subdirectory(vendor)


set(SOURCES
		src/vk_manager.cpp
		src/vk_types.cpp
		src/imgui_theme.cpp
		src/logger.cpp
		src/vk_descriptors.cpp
		src/vk_pipeline.cpp
		src/vk_engine.cpp
		src/vk_initializers.cpp
		src/vk_scene.cpp
		src/vk_shader.cpp
		src/vk_textures.cpp
	)

set(HEADERS
		src/utils.h

		src/vk_manager.h
		src/vk_types.h
		src/imgui_theme.h
		src/logger.h
		src/vk_descriptors.h
		src/vk_pipeline.h
		src/vk_engine.h
		src/vk_initializers.h
		src/vk_scene.h
		src/vk_shader.h
		src/vk_textures.h
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

else()
	add_custom_command(
		TARGET ${PROJECT_NAME}
		COMMAND 
		${CMAKE_COMMAND} -E create_symlink 
		${CMAKE_SOURCE_DIR}/${RES_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/${RES_DIR}
		)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

message(STATUS ${Vulkan_INCLUDE_DIRS})


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
		$<$<CONFIG:Release>:${SHADERC}>
		$<$<CONFIG:Debug>:${SHADERC_D}>
		$<$<CONFIG:Release>:${SPIRV_CROSS}>
		$<$<CONFIG:Debug>:${SPIRV_CROSS_D}>
		)

else()

	message(STATUS ${SPIRV_CROSS})

	target_link_libraries(${PROJECT_NAME} 
		${SHADERC}
		${SPIRV_CROSS}
		)
endif()

target_link_libraries(${PROJECT_NAME} 
	Vulkan::Vulkan 
	glfw
	vk-bootstrap::vk-bootstrap
	VulkanMemoryAllocator
	spdlog
	glm
	stb_image
	tinyobjloader
	imgui
	)
