cmake_minimum_required(VERSION 3.10)

set(PROJ_NAME Application)
project(${PROJ_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(Vulkan REQUIRED FATAL_ERROR)

add_subdirectory(vendor)

set(SOURCES ${SOURCES}
    src/main.cpp
    )

add_executable(${PROJ_NAME} ${SOURCES})

target_include_directories(${PROJ_NAME}
    PUBLIC ${VULKAN_INCLUDE_DIRS}
)

target_link_libraries (${PROJ_NAME} ${VULKAN_LIB_LIST})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

target_link_libraries (${PROJ_NAME} glfw)
