#pragma once

namespace Utils {

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
