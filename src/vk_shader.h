#pragma once

#include <vulkan/vulkan_core.h>

namespace vkutil {

bool compile_shader_module(uint32_t *buffer, uint32_t byteSize,
                           VkShaderModule *outShaderModule,
                           const VkDevice device);

bool load_spirv_shader_module(const char *filePath,
                              VkShaderModule *outShaderModule,
                              const VkDevice device);

bool load_glsl_shader(std::filesystem::path filePath,
                             ShaderType type,
                             VkShaderModule *outShaderModule,
                             const VkDevice device);

bool load_glsl_shader(std::filesystem::path filePath,
                             VkShaderModule *outShaderModule,
                             const VkDevice device);
} // namespace vkutil
