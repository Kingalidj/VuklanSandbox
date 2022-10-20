cmake_minimum_required(VERSION 3.10)

project(Application)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE 1)

# add_compile_options( -g )

find_package(Vulkan REQUIRED FATAL_ERROR)

add_subdirectory(vendor)

set(resources res)


add_executable(${PROJECT_NAME} 
	src/main.cpp
	src/vk_engine.cpp
	src/vk_initializers.cpp
	src/logger.cpp
	src/vk_scene.cpp
	src/vk_textures.cpp
	src/vk_shader.cpp
	)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE src/pch.h
	)

# target_compile_options(${PROJECT_NAME} PRIVATE "-Wall")

macro(print_all_variables)
	message(STATUS "print_all_variables------------------------------------------{")
	get_cmake_property(_variableNames VARIABLES)
	foreach (_variableName ${_variableNames})
		message(STATUS "${_variableName}=${${_variableName}}")
	endforeach()
	message(STATUS "print_all_variables------------------------------------------}")
endmacro()

if (WIN32)
	get_filename_component(real_path "${resources}" REALPATH)
	string(REPLACE "/" "\\" target_path "${real_path}")

	add_custom_command( #link_resources ALL
		TARGET ${PROJECT_NAME}
		COMMAND cmd /C if not exist "${resources}" mklink /J ${resources} "${target_path}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		)
else()
	add_custom_command(
		TARGET ${PROJECT_NAME}
		COMMAND 
		${CMAKE_COMMAND} -E create_symlink 
		${CMAKE_SOURCE_DIR}/${resources} $<TARGET_FILE_DIR:${PROJECT_NAME}>/${resources}
		)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})


target_include_directories(${PROJECT_NAME}
	PUBLIC ${VULKAN_INCLUDE_DIRS}
	)

get_filename_component(VULKAN_LIB ${Vulkan_LIBRARY} DIRECTORY)

find_library(SHADERC_LIB shaderc_combined
	HINTS ${VULKAN_LIB}
	)

target_link_libraries(${PROJECT_NAME} 
	Vulkan::Vulkan 
	${SHADERC_LIB}
	glfw
	vk-bootstrap::vk-bootstrap
	VulkanMemoryAllocator
	spdlog
	glm
	stb_image
	tinyobjloader
	)
