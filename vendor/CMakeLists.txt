set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw)

# == vk-bootstrap ==
add_subdirectory(vk-bootstrap)

# == glfw ==

add_subdirectory(glfw)
include_directories(glfw/include)
