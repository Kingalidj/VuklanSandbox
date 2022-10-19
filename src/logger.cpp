#include "logger.h"

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;
std::ostringstream Logger::s_OStream;

void Logger::init() {
  auto ostreamSink =
      std::make_shared<spdlog::sinks::ostream_sink_st>(s_OStream);

  spdlog::set_pattern("%^%n [%l]: %v%$");

  s_CoreLogger = spdlog::stdout_color_mt("CORE");
  s_CoreLogger->set_level(spdlog::level::warn);

  s_ClientLogger = spdlog::stdout_color_mt("APP");
  s_ClientLogger->set_level(spdlog::level::trace);
}
