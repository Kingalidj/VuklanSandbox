#include "vk_shader.h"

#include <shaderc/shaderc.hpp>

namespace vkutil {

bool compile_shader_module(uint32_t *buffer, uint32_t byteSize,
                           VkShaderModule *outShaderModule,
                           const VkDevice device) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;

  createInfo.codeSize = byteSize;
  createInfo.pCode = buffer;

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    return false;
  }

  *outShaderModule = shaderModule;
  return true;
}

bool load_spirv_shader_module(const char *filePath,
                              VkShaderModule *outShaderModule,
                              const VkDevice device) {
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    CORE_WARN("Could not open spv file: {}", filePath);
    return false;
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  file.seekg(0);
  file.read((char *)buffer.data(), fileSize);
  file.close();

  if (!compile_shader_module(buffer.data(), buffer.size() * sizeof(uint32_t),
                             outShaderModule, device)) {
    CORE_WARN("Could not compile shader: {}", filePath);
    return false;
  }

  return true;
}

bool load_glsl_shader_module(std::filesystem::path filePath,
                             ShaderType type,
                             VkShaderModule *outShaderModule,
                             const VkDevice device) {

  shaderc_shader_kind kind;

  switch (type) {
  case ShaderType::Vertex:
    kind = shaderc_vertex_shader;
    break;
  case ShaderType::Fragment:
    kind = shaderc_fragment_shader;
    break;
  }

  std::ifstream file(filePath, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    CORE_WARN("Could not open file: {}", filePath);
    return false;
  }

  size_t fileSize = (size_t)file.tellg();
  /* std::vector<char> buffer(fileSize / sizeof(char)); */
  std::string source;
  source.resize(fileSize / sizeof(char));

  file.seekg(0);
  file.read((char *)source.data(), fileSize);
  file.close();

  shaderc::CompileOptions cOptions;
  cOptions.SetTargetEnvironment(shaderc_target_env_vulkan,
                                shaderc_env_version_vulkan_1_1);
  const bool optimize = false;

  if (optimize)
    cOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

  shaderc::Compiler compiler;

  shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
      source, kind, (char *)filePath.c_str(), cOptions);
  if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cerr << module.GetErrorMessage() << std::endl;
    return false;
  }

  std::vector<uint32_t> buffer =
      std::vector<uint32_t>(module.cbegin(), module.cend());

  if (!compile_shader_module(buffer.data(), buffer.size() * sizeof(uint32_t),

                             outShaderModule, device)) {
    CORE_WARN("Could not compile shader: {}", filePath);
    return false;
  }
  return true;
}
bool load_glsl_shader_module(std::filesystem::path filePath,
                             VkShaderModule *outShaderModule,
                             const VkDevice device) {
  auto ext = filePath.extension();

  ShaderType type;
  if (ext == ".vert") {
    type = ShaderType::Vertex;
  } else if (ext == ".frag") {
    type = ShaderType::Fragment;
  } else {
    CORE_WARN("unknown extension for file: {}", filePath);
    return false;
  }

  return load_glsl_shader_module(filePath, type, outShaderModule, device);
}

} // namespace vkutil
