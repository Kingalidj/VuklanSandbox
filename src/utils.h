#pragma once

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      CORE_ERROR("at line: {}, in file: {}\nDetected Vulkan error: {}",        \
                 __LINE__, __FILE__, err);                                     \
    }                                                                          \
  } while (0)

enum class ShaderType {
	Fragment,
	Vertex,
};

template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}
