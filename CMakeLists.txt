cmake_minimum_required(VERSION 3.10)

project(Application)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE 1)

# add_compile_options( -Wall )

find_package(Vulkan REQUIRED FATAL_ERROR)

add_subdirectory(vendor)

add_executable(${PROJECT_NAME} 
	src/main.cpp
	src/vk_engine.cpp
	src/vk_initializers.cpp
	src/logger.cpp
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

add_custom_target(
	copy_resources ALL
	DEPENDS copy_command
	)

add_custom_command(
	OUTPUT
	copy_command
	COMMAND 
	${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/res/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/

	DEPENDS
	always_rebuild
	)

add_custom_command(
	OUTPUT always_rebuild
	COMMAND cmake -E echo
	)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})


target_include_directories(${PROJECT_NAME}
	PUBLIC ${VULKAN_INCLUDE_DIRS}
	)

get_filename_component(VULKAN_LIB ${Vulkan_LIBRARY} DIRECTORY)

message(${VULKAN_LIB})

find_library(SHADERC_LIB shaderc_combined
	HINTS ${VULKAN_LIB}
	)

message(${VULKAN_LIB})

target_link_libraries(${PROJECT_NAME} 
	Vulkan::Vulkan 
	${SHADERC_LIB}
	glfw
	vk-bootstrap::vk-bootstrap
	spdlog
	)
