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
