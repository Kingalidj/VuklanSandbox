#pragma once

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      CORE_ERROR("at line: {}, in file: {}\nDetected Vulkan error: {}",        \
                 __LINE__, __FILE__, err);                                     \
    }                                                                          \
  } while (0)

namespace vkutil {

enum class LogLevel {
  Trace,
  Info,
  Warn,
  Error,
};

enum class ShaderType {
  Fragment,
  Vertex,
};

enum class QueueType {
  Present,
  Graphics,
  Compute,
  Transfer,
};

} // namespace Utils
