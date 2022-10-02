#include "vk_engine.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "vk_initializers.h"
#include "vk_types.h"

#include "VkBootstrap.h"

#include <shaderc/shaderc.hpp>

#include <cmath>

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      CORE_ERROR("at line: {}, in file: {}\nDetected Vulkan error: {}",        \
                 __LINE__, __FILE__, err)                                      \
      abort();                                                                 \
    }                                                                          \
  } while (0)

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
                             VkShaderModule *outShaderModule,
                             const VkDevice device) {
  auto ext = filePath.extension();

  Utils::ShaderType type;
  if (ext == ".vert") {
    type = Utils::ShaderType::VERTEX;
  } else if (ext == ".frag") {
    type = Utils::ShaderType::FRAGMENT;
  } else {
    CORE_WARN("unknown extension for file: {}", filePath);
    return false;
  }

  return load_glsl_shader_module(filePath, type, outShaderModule, device);
}

bool load_glsl_shader_module(std::filesystem::path filePath,
                             Utils::ShaderType type,
                             VkShaderModule *outShaderModule,
                             const VkDevice device) {

  shaderc_shader_kind kind;

  switch (type) {
  case Utils::ShaderType::VERTEX:
    kind = shaderc_vertex_shader;
    break;
  case Utils::ShaderType::FRAGMENT:
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
  const bool optimize = true;

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

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = nullptr;

  viewportState.viewportCount = 1;
  viewportState.pViewports = &m_Viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &m_Scissor;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = nullptr;

  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &m_ColorBlendAttachment;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = nullptr;

  pipelineInfo.stageCount = m_ShaderStages.size();
  pipelineInfo.pStages = m_ShaderStages.data();
  pipelineInfo.pVertexInputState = &m_VertexInputInfo;
  pipelineInfo.pInputAssemblyState = &m_InputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &m_Rasterizer;
  pipelineInfo.pMultisampleState = &m_Multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = m_PipelineLayout;
  pipelineInfo.renderPass = pass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  VkPipeline pipeLine;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &pipeLine) != VK_SUCCESS) {
    std::cerr << "failed to create pipeline" << std::endl;
    return VK_NULL_HANDLE;
  } else {
    return pipeLine;
  }
}

void VulkanEngine::init() {

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  m_Window = glfwCreateWindow(m_WindowExtent.width, m_WindowExtent.height,
                              "Vulkan Engine", nullptr, nullptr);

  init_vulkan();
  init_swapchain();
  init_commands();
  init_default_renderpass();
  init_framebuffers();
  init_sync_structures();
  init_pipelines();

  m_IsInitialized = true;
}

void VulkanEngine::cleanup() {
  if (m_IsInitialized) {

    vkWaitForFences(m_Device, 1, &m_RenderFence, true, 1000000000);

    m_MainDeletionQueue.flush();

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyDevice(m_Device, nullptr);
    vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    vkDestroyInstance(m_Instance, nullptr);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
  }
}

void VulkanEngine::draw() {
  // wait for last frame to finish. Timeout of 1 second
  VK_CHECK(vkWaitForFences(m_Device, 1, &m_RenderFence, true, 1000000000));
  VK_CHECK(vkResetFences(m_Device, 1, &m_RenderFence));

  // since we waited the buffer is empty
  VK_CHECK(vkResetCommandBuffer(m_MainCommandBuffer, 0));

  // we will write to this image index (framebuffer)
  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000,
                                 m_PresentSemaphore, nullptr,
                                 &swapchainImageIndex));

  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(m_MainCommandBuffer, &cmdBeginInfo));

  VkClearValue clearValue = {};
  float flash = 1 - std::abs(std::sin(m_FrameNumber / 120.f));
  clearValue.color = {{0.0f, 0.0f, flash, 1.0f}};

  // start main renderpass
  VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(
      m_RenderPass, m_WindowExtent, m_FrameBuffers[swapchainImageIndex]);

  rpInfo.clearValueCount = 1;
  rpInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(m_MainCommandBuffer, &rpInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(m_MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_TrianglePipeline);
  vkCmdDraw(m_MainCommandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(m_MainCommandBuffer);
  VK_CHECK(vkEndCommandBuffer(m_MainCommandBuffer));

  VkSubmitInfo submitInfo = vkinit::submit_info(&m_MainCommandBuffer);

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  submitInfo.pWaitDstStageMask = &waitStage;

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &m_PresentSemaphore;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_RenderSemaphore;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_MainCommandBuffer;

  VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_RenderFence));

  VkPresentInfoKHR presentInfo = vkinit::present_info();

  presentInfo.pSwapchains = &m_Swapchain;
  presentInfo.swapchainCount = 1;

  presentInfo.pWaitSemaphores = &m_RenderSemaphore;
  presentInfo.waitSemaphoreCount = 1;

  presentInfo.pImageIndices = &swapchainImageIndex;

  VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

  m_FrameNumber++;
}

void VulkanEngine::run() {

  while (!glfwWindowShouldClose(m_Window)) {
    glfwPollEvents();

    draw();
  }
}

VkBool32
spdlog_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT msgType,
                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                      void *) {
  auto mt = vkb::to_string_message_type(msgType);

  switch (msgSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    CORE_TRACE("({}) {}", mt, pCallbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    CORE_INFO("({}) {}", mt, pCallbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    CORE_WARN("({}) {}", mt, pCallbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    CORE_ERROR("({}) {}", mt, pCallbackData->pMessage);
    break;
  }

  return VK_FALSE;
}

void VulkanEngine::init_vulkan() {
  vkb::Instance vkb_inst =
      vkb::InstanceBuilder()
          .set_app_name("Vulkan Application")
          .request_validation_layers(true)
          .require_api_version(1, 1, 0)
          .set_debug_callback(spdlog_debug_callback)
          /* .use_default_debug_messenger() */
          //          .set_debug_messenger_severity(
          // VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
          .build()
          .value();

  m_Instance = vkb_inst.instance;
  m_DebugMessenger = vkb_inst.debug_messenger;

  VK_CHECK(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface));

  vkb::PhysicalDevice physicalDevice = vkb::PhysicalDeviceSelector(vkb_inst)
                                           .set_minimum_version(1, 1)
                                           .set_surface(m_Surface)
                                           .select()
                                           .value();

  vkb::Device vkbDevice = vkb::DeviceBuilder(physicalDevice).build().value();

  m_Device = vkbDevice.device;
  m_ChosenGPU = physicalDevice.physical_device;

  m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  m_GraphicsQueueFamily =
      vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::init_swapchain() {

  vkb::Swapchain vkbSwapchain =
      vkb::SwapchainBuilder(m_ChosenGPU, m_Device, m_Surface)
          .use_default_format_selection()
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_extent(m_WindowExtent.width, m_WindowExtent.height)
          .build()
          .value();

  m_Swapchain = vkbSwapchain.swapchain;
  m_SwapchainImages = vkbSwapchain.get_images().value();
  m_SwapchainImageViews = vkbSwapchain.get_image_views().value();

  m_SwapchainImageFormat = vkbSwapchain.image_format;

  m_MainDeletionQueue.push_function(
      [=]() { vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr); });
}

void VulkanEngine::init_commands() {
  VkCommandPoolCreateInfo cmdPoolInfo = vkinit::command_pool_create_info(
      m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  VK_CHECK(
      vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr, &m_CommandPool));

  VkCommandBufferAllocateInfo cmdAllocInfo =
      vkinit::command_buffer_allocate_info(m_CommandPool, 1);
  VK_CHECK(
      vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_MainCommandBuffer));

  m_MainDeletionQueue.push_function(
      [=]() { vkDestroyCommandPool(m_Device, m_CommandPool, nullptr); });
}

void VulkanEngine::init_default_renderpass() {
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = m_SwapchainImageFormat;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {};
  // attachment number will index into the pAttachments array in the
  // parent renderpass
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;

  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;

  VK_CHECK(
      vkCreateRenderPass(m_Device, &render_pass_info, nullptr, &m_RenderPass));

  m_MainDeletionQueue.push_function(
      [=]() { vkDestroyRenderPass(m_Device, m_RenderPass, nullptr); });
}

void VulkanEngine::init_framebuffers() {
  VkFramebufferCreateInfo fb_info =
      vkinit::framebuffer_create_info(m_RenderPass, m_WindowExtent);

  const uint32_t swapchain_imagecount = m_SwapchainImages.size();
  m_FrameBuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

  // create framebuffers for each of the swapchain image views
  for (uint32_t i = 0; i < swapchain_imagecount; i++) {
    fb_info.pAttachments = &m_SwapchainImageViews[i];
    VK_CHECK(
        vkCreateFramebuffer(m_Device, &fb_info, nullptr, &m_FrameBuffers[i]));

    m_MainDeletionQueue.push_function([=]() {
      vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
      vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
    });
  }
}

void VulkanEngine::init_sync_structures() {
  VkFenceCreateInfo fenceCreateInfo =
      vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_RenderFence));

  m_MainDeletionQueue.push_function(
      [=]() { vkDestroyFence(m_Device, m_RenderFence, nullptr); });

  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
  VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
                             &m_PresentSemaphore));
  VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
                             &m_RenderSemaphore));

  m_MainDeletionQueue.push_function([=]() {
    vkDestroySemaphore(m_Device, m_PresentSemaphore, nullptr);
    vkDestroySemaphore(m_Device, m_RenderSemaphore, nullptr);
  });
}

void VulkanEngine::init_pipelines() {

  VkShaderModule triangleFragShader;
  if (!load_glsl_shader_module("res/shaders/triangle.frag", &triangleFragShader,
                               m_Device)) {
    return;
  } else {
    CORE_INFO("Triangle fragment shader successfully loaded")
  }

  VkShaderModule triangleVertexShader;
  if (!load_glsl_shader_module("res/shaders/triangle.vert",
                               Utils::ShaderType::VERTEX, &triangleVertexShader,
                               m_Device)) {
    return;
  } else {
    CORE_INFO("Triangle vertex shader successfully loaded")
  }

  VkPipelineLayoutCreateInfo pipeline_layout_info =
      vkinit::pipeline_layout_create_info();

  VK_CHECK(vkCreatePipelineLayout(m_Device, &pipeline_layout_info, nullptr,
                                  &m_TrianglePipelineLayout));

  PipelineBuilder pipelineBuilder;

  pipelineBuilder.m_ShaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT,
                                                triangleVertexShader));

  pipelineBuilder.m_ShaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT,
                                                triangleFragShader));

  pipelineBuilder.m_VertexInputInfo = vkinit::vertex_input_state_create_info();
  pipelineBuilder.m_InputAssembly =
      vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  pipelineBuilder.m_Viewport.x = 0.0f;
  pipelineBuilder.m_Viewport.y = 0.0f;
  pipelineBuilder.m_Viewport.width = (float)m_WindowExtent.width;
  pipelineBuilder.m_Viewport.height = (float)m_WindowExtent.height;
  pipelineBuilder.m_Viewport.minDepth = 0.0f;
  pipelineBuilder.m_Viewport.maxDepth = 1.0f;

  pipelineBuilder.m_Scissor.offset = {0, 0};
  pipelineBuilder.m_Scissor.extent = m_WindowExtent;

  pipelineBuilder.m_Rasterizer =
      vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
  pipelineBuilder.m_Multisampling = vkinit::multisampling_state_create_info();
  pipelineBuilder.m_ColorBlendAttachment =
      vkinit::color_blend_attachment_state();
  pipelineBuilder.m_PipelineLayout = m_TrianglePipelineLayout;

  m_TrianglePipeline = pipelineBuilder.build_pipeline(m_Device, m_RenderPass);

  vkDestroyShaderModule(m_Device, triangleFragShader, nullptr);
  vkDestroyShaderModule(m_Device, triangleVertexShader, nullptr);

  m_MainDeletionQueue.push_function([=]() {
    vkDestroyPipeline(m_Device, m_TrianglePipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_TrianglePipelineLayout, nullptr);
  });
}
