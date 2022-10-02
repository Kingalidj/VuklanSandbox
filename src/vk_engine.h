#pragma once

#include "vk_types.h"

struct DeletionQueue {

  std::deque<std::function<void()>> deletors;

  void push_function(std::function<void()> &&func) { deletors.push_back(func); }

  void flush() {
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
      (*it)();
    }

    deletors.clear();
  }
};

class PipelineBuilder {

public:
  std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
  VkPipelineVertexInputStateCreateInfo m_VertexInputInfo;
  VkPipelineInputAssemblyStateCreateInfo m_InputAssembly;
  VkViewport m_Viewport;
  VkRect2D m_Scissor;
  VkPipelineRasterizationStateCreateInfo m_Rasterizer;
  VkPipelineColorBlendAttachmentState m_ColorBlendAttachment;
  VkPipelineMultisampleStateCreateInfo m_Multisampling;
  VkPipelineLayout m_PipelineLayout;

  VkPipeline build_pipeline(VkDevice device, VkRenderPass renderPass);
};

bool compile_shader_module(uint32_t *buffer, uint32_t byteSize,
                           VkShaderModule *outShaderModule,
                           const VkDevice device);

bool load_spirv_shader_module(const char *filePath,
                              VkShaderModule *outShaderModule,
                              const VkDevice device);

bool load_glsl_shader_module(std::filesystem::path filePath,
                             Utils::ShaderType type,
                             VkShaderModule *outShaderModule,
                             const VkDevice device);
bool load_glsl_shader_module(std::filesystem::path filePath,
                             VkShaderModule *outShaderModule,
                             const VkDevice device);

class VulkanEngine {
public:
  bool m_IsInitialized{false};
  int m_FrameNumber{0};

  VkExtent2D m_WindowExtent{800, 600};

  struct GLFWwindow *m_Window = nullptr;

  void init();
  void cleanup();
  void draw();
  void run();

  VkInstance m_Instance;
  VkDebugUtilsMessengerEXT m_DebugMessenger;
  VkPhysicalDevice m_ChosenGPU;
  VkDevice m_Device;
  VkSurfaceKHR m_Surface; // vulkan window

  VkSwapchainKHR m_Swapchain;
  VkFormat m_SwapchainImageFormat;

  std::vector<VkImage> m_SwapchainImages;
  std::vector<VkImageView> m_SwapchainImageViews;

  VkQueue m_GraphicsQueue;
  uint32_t m_GraphicsQueueFamily;

  VkCommandPool m_CommandPool;
  VkCommandBuffer m_MainCommandBuffer;

  VkRenderPass m_RenderPass;
  std::vector<VkFramebuffer> m_FrameBuffers;

  VkSemaphore m_PresentSemaphore, m_RenderSemaphore;
  VkFence m_RenderFence;

  VkPipelineLayout m_TrianglePipelineLayout;

  VkPipeline m_TrianglePipeline;

  DeletionQueue m_MainDeletionQueue;

private:
  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_default_renderpass();
  void init_framebuffers();
  void init_sync_structures();
  void init_pipelines();
};
