#include "vk_engine.h"

#include "vk_initializers.h"
#include "vk_shader.h"
#include "vk_types.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <VkBootstrap.h>
#include <glm/gtx/transform.hpp>
#include <shaderc/shaderc.hpp>

#include <cmath>

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      CORE_ERROR("at line: {}, in file: {}\nDetected Vulkan error: {}",        \
                 __LINE__, __FILE__, err);                                     \
    }                                                                          \
  } while (0)

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
  init_descriptors();
  init_pipelines();

  load_meshes();
  init_scene();

  m_IsInitialized = true;
}

Material *VulkanEngine::create_material(VkPipeline pipeline,
                                        VkPipelineLayout layout,
                                        const std::string &name) {
  Material mat;
  mat.pipeline = pipeline;
  mat.pipelineLayout = layout;
  m_Materials[name] = mat;
  return &m_Materials[name];
}

Material *VulkanEngine::get_material(const std::string &name) {
  auto it = m_Materials.find(name);
  if (it == m_Materials.end())
    return nullptr;
  return &(*it).second;
}

Mesh *VulkanEngine::get_mesh(const std::string &name) {
  auto it = m_Meshes.find(name);
  if (it == m_Meshes.end())
    return nullptr;
  return &(*it).second;
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject *first,
                                int count) {
  glm::vec3 camPos = {0.f, -3.f, -8.f};

  glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
  glm::mat4 projection = glm::perspective(
      glm::radians(70.f),
      (float)m_WindowExtent.width / (float)m_WindowExtent.height, 0.1f, 200.0f);
  projection[1][1] *= -1;

  glm::mat4 rotMat =
      glm::rotate(glm::mat4(1), glm::radians((float)m_FrameNumber * 2.0f),
                  glm::vec3(0, 1, 0));

  GPUCameraData camData;
  camData.proj = projection;
  camData.view = view;
  camData.viewProj = projection * view;

  void *data;
  vmaMapMemory(m_Allocator, get_current_frame().cameraBuffer.allocation, &data);
  memcpy(data, &camData, sizeof(GPUCameraData));
  vmaUnmapMemory(m_Allocator, get_current_frame().cameraBuffer.allocation);

  float framed = (m_FrameNumber / 120.f);
  m_SceneParameters.ambientColor = {sin(framed), 0, cos(framed), 1};

  char *sceneData;
  vmaMapMemory(m_Allocator, m_SceneParameterBuffer.allocation,
               (void **)&sceneData);

  int frameIndex = m_FrameNumber % FRAME_OVERLAP;
  sceneData += pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
  memcpy(sceneData, &m_SceneParameters, sizeof(GPUSceneData));
  vmaUnmapMemory(m_Allocator, m_SceneParameterBuffer.allocation);

  void *objectData;
  vmaMapMemory(m_Allocator, get_current_frame().objectBuffer.allocation,
               &objectData);

  GPUObjectData *objectSSBO = (GPUObjectData *)objectData;

  for (int i = 0; i < count; i++) {
    RenderObject &object = first[i];
    objectSSBO[i].modelMatrix = object.transformMatrix;
  }

  vmaUnmapMemory(m_Allocator, get_current_frame().objectBuffer.allocation);

  Mesh *lastMesh = nullptr;
  Material *lastMaterial = nullptr;
  for (int i = 0; i < count; i++) {
    RenderObject &object = first[i];

    if (object.material != lastMaterial) {

      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        object.material->pipeline);
      lastMaterial = object.material;

      uint32_t uniformOffset =
          pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;

      vkCmdBindDescriptorSets(
          cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout,
          0, 1, &get_current_frame().globalDescriptor, 1, &uniformOffset);

      vkCmdBindDescriptorSets(
          cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout,
          1, 1, &get_current_frame().objectDescriptor, 0, nullptr);
    }

    /* glm::mat4 model = object.transformMatrix * rotMat; */
    /* glm::mat4 meshMatrix = projection * view * model; */

    MeshPushConstants constants;
    constants.renderMatrix = object.transformMatrix * rotMat;

    // upload the mesh to the GPU via push constants
    vkCmdPushConstants(cmd, object.material->pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants),
                       &constants);

    if (object.mesh != lastMesh) {
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer,
                             &offset);
      lastMesh = object.mesh;
    }

    vkCmdDraw(cmd, (uint32_t)object.mesh->vertices.size(), 1, 0, i); //TODO: look at first instance
  }
}

void VulkanEngine::cleanup() {
  if (m_IsInitialized) {

    /* vkWaitForFences(m_Device, 1, &get_current_frame().renderFence, true, */
    /*                 1000000000); */
    vkDeviceWaitIdle(m_Device);

    m_MainDeletionQueue.flush();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
  }
}

void VulkanEngine::draw() {
  // wait for last frame to finish. Timeout of 1 second
  VK_CHECK(vkWaitForFences(m_Device, 1, &get_current_frame().renderFence, true,
                           1000000000));
  VK_CHECK(vkResetFences(m_Device, 1, &get_current_frame().renderFence));

  // since we waited the buffer is empty
  VK_CHECK(vkResetCommandBuffer(get_current_frame().mainCommandBuffer, 0));

  // we will write to this image index (framebuffer)
  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000,
                                 get_current_frame().presentSemaphore, nullptr,
                                 &swapchainImageIndex));

  VkCommandBuffer cmd = get_current_frame().mainCommandBuffer;

  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  VkClearValue clearValue{};
  clearValue.color = {{1.0f, 1.0f, 1.0, 1.0f}};

  VkClearValue depthClear;
  depthClear.depthStencil.depth = 1.f;

  // start main renderpass
  VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(
      m_RenderPass, m_WindowExtent, m_FrameBuffers[swapchainImageIndex]);

  rpInfo.clearValueCount = 2;
  VkClearValue clearValues[] = {clearValue, depthClear};
  /* VkClearValue clearValues[] = {clearValue, depthClear}; */
  rpInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  draw_objects(cmd, m_RenderObjects.data(), (int)m_RenderObjects.size());

  vkCmdEndRenderPass(cmd);
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkSubmitInfo submitInfo = vkinit::submit_info(&cmd);

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  submitInfo.pWaitDstStageMask = &waitStage;

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &get_current_frame().presentSemaphore;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &get_current_frame().renderSemaphore;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo,
                         get_current_frame().renderFence));

  VkPresentInfoKHR presentInfo = vkinit::present_info();

  presentInfo.pSwapchains = &m_Swapchain;
  presentInfo.swapchainCount = 1;

  presentInfo.pWaitSemaphores = &get_current_frame().renderSemaphore;
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
  vkb::Instance vkb_inst = vkb::InstanceBuilder()
                               .set_app_name("Vulkan Application")
                               .request_validation_layers(true)
                               .require_api_version(1, 1, 0)
                               .set_debug_callback(spdlog_debug_callback)
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

  VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
  shaderDrawParametersFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
  shaderDrawParametersFeatures.pNext = nullptr;
  shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;
  vkb::Device vkbDevice = vkb::DeviceBuilder(physicalDevice)
                              .add_pNext(&shaderDrawParametersFeatures)
                              .build()
                              .value();

  m_GPUProperties = vkbDevice.physical_device.properties;
  CORE_INFO("The GPU has a minimum buffer alignment of {}",
            m_GPUProperties.limits.minUniformBufferOffsetAlignment);

  m_Device = vkbDevice.device;
  m_ChosenGPU = physicalDevice.physical_device;

  m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  m_GraphicsQueueFamily =
      vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  VmaAllocatorCreateInfo allocatorInfo{};
  allocatorInfo.physicalDevice = m_ChosenGPU;
  allocatorInfo.device = m_Device;
  allocatorInfo.instance = m_Instance;
  vmaCreateAllocator(&allocatorInfo, &m_Allocator);

  m_MainDeletionQueue.push_function([=]() {
    vmaDestroyAllocator(m_Allocator);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyDevice(m_Device, nullptr);
    vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    vkDestroyInstance(m_Instance, nullptr);
  });
}

void VulkanEngine::init_swapchain() {

  vkb::Swapchain vkbSwapchain =
      vkb::SwapchainBuilder(m_ChosenGPU, m_Device, m_Surface)
          .use_default_format_selection()
          // set vsync
          .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
          //.set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
          .set_desired_extent(m_WindowExtent.width, m_WindowExtent.height)
          .build()
          .value();

  m_Swapchain = vkbSwapchain.swapchain;
  m_SwapchainImages = vkbSwapchain.get_images().value();
  m_SwapchainImageViews = vkbSwapchain.get_image_views().value();

  m_SwapchainImageFormat = vkbSwapchain.image_format;

  VkExtent3D depthImageExtent = {m_WindowExtent.width, m_WindowExtent.height,
                                 1};

  m_DepthFormat = VK_FORMAT_D32_SFLOAT;

  VkImageCreateInfo dimgInfo = vkinit::image_create_info(
      m_DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      depthImageExtent);

  VmaAllocationCreateInfo dimgAllocinfo{};
  dimgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  dimgAllocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // TODO:

  vmaCreateImage(m_Allocator, &dimgInfo, &dimgAllocinfo, &m_DepthImage.image,
                 &m_DepthImage.allocation, nullptr);

  VkImageViewCreateInfo dviewInfo = vkinit::imageview_create_info(
      m_DepthFormat, m_DepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

  VK_CHECK(vkCreateImageView(m_Device, &dviewInfo, nullptr, &m_DepthImageView));

  m_MainDeletionQueue.push_function([=]() {
    vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
  });
}

void VulkanEngine::init_commands() {
  VkCommandPoolCreateInfo cmdPoolInfo = vkinit::command_pool_create_info(
      m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr,
                                 &m_Frames[i].commandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo =
        vkinit::command_buffer_allocate_info(m_Frames[i].commandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo,
                                      &m_Frames[i].mainCommandBuffer));

    m_MainDeletionQueue.push_function([=]() {
      vkDestroyCommandPool(m_Device, m_Frames[i].commandPool, nullptr);
    });
  }

  /* VK_CHECK( */
  /*     vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr, &m_CommandPool));
   */

  /* VkCommandBufferAllocateInfo cmdAllocInfo = */
  /*     vkinit::command_buffer_allocate_info(m_CommandPool, 1); */
  /* VK_CHECK( */
  /*     vkAllocateCommandBuffers(m_Device, &cmdAllocInfo,
   * &m_MainCommandBuffer)); */

  /* m_MainDeletionQueue.push_function( */
  /*     [=]() { vkDestroyCommandPool(m_Device, m_CommandPool, nullptr); }); */
}

void VulkanEngine::init_default_renderpass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = m_SwapchainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  // attachment number will index into the pAttachments array in the
  // parent renderpass
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.flags = 0;
  depthAttachment.format = m_DepthFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDependency depthDependency = {};
  depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  depthDependency.dstSubpass = 0;
  depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depthDependency.srcAccessMask = 0;
  depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
  VkSubpassDependency dependencies[2] = {dependency, depthDependency};

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 2;
  renderPassInfo.pAttachments = &attachments[0];
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies = &dependencies[0];

  VK_CHECK(
      vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));

  m_MainDeletionQueue.push_function(
      [=]() { vkDestroyRenderPass(m_Device, m_RenderPass, nullptr); });
}

void VulkanEngine::init_framebuffers() {
  VkFramebufferCreateInfo fbInfo =
      vkinit::framebuffer_create_info(m_RenderPass, m_WindowExtent);

  const uint32_t swapchainImagecount = (uint32_t)m_SwapchainImages.size();
  m_FrameBuffers = std::vector<VkFramebuffer>(swapchainImagecount);

  // create framebuffers for each of the swapchain image views
  for (uint32_t i = 0; i < swapchainImagecount; i++) {
    VkImageView attachments[2];
    attachments[0] = m_SwapchainImageViews[i];
    attachments[1] = m_DepthImageView;

    fbInfo.pAttachments = attachments;
    fbInfo.attachmentCount = 2;

    VK_CHECK(
        vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &m_FrameBuffers[i]));

    m_MainDeletionQueue.push_function([=]() {
      vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
      vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
    });
  }
}

void VulkanEngine::init_sync_structures() {
  VkFenceCreateInfo fenceCreateInfo =
      vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

  for (int i = 0; i < FRAME_OVERLAP; i++) {

    VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr,
                           &m_Frames[i].renderFence));

    m_MainDeletionQueue.push_function(
        [=]() { vkDestroyFence(m_Device, m_Frames[i].renderFence, nullptr); });

    VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
                               &m_Frames[i].presentSemaphore));
    VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
                               &m_Frames[i].renderSemaphore));

    m_MainDeletionQueue.push_function([=]() {
      vkDestroySemaphore(m_Device, m_Frames[i].presentSemaphore, nullptr);
      vkDestroySemaphore(m_Device, m_Frames[i].renderSemaphore, nullptr);
    });
  }
}

void VulkanEngine::init_pipelines() {

  VkShaderModule triangleFragShader;
  if (!load_glsl_shader_module("res/shaders/triangle.frag", &triangleFragShader,
                               m_Device)) {
    return;
  } else {
    CORE_INFO("Triangle fragment shader successfully loaded");
  }

  VkShaderModule triangleVertexShader;
  if (!load_glsl_shader_module("res/shaders/triangle.vert",
                               utils::ShaderType::Vertex, &triangleVertexShader,
                               m_Device)) {
    return;
  } else {
    CORE_INFO("Triangle vertex shader successfully loaded");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo =
      vkinit::pipeline_layout_create_info();

  VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr,
                                  &m_TrianglePipelineLayout));

  VkPipelineLayoutCreateInfo meshPipelineLayoutInfo =
      vkinit::pipeline_layout_create_info();

  VkPushConstantRange pushConstant;
  pushConstant.offset = 0;
  pushConstant.size = sizeof(MeshPushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayout setLayouts[] = {m_GlobalSetLayout, m_ObjectSetLayout};

  meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
  meshPipelineLayoutInfo.pushConstantRangeCount = 1;
  meshPipelineLayoutInfo.setLayoutCount = 2;
  meshPipelineLayoutInfo.pSetLayouts = setLayouts;

  VK_CHECK(vkCreatePipelineLayout(m_Device, &meshPipelineLayoutInfo, nullptr,
                                  &m_MeshPipelineLayout));

  VertexInputDescription vertexDescription = Vertex::get_vertex_description();

  m_MeshPipeline =
      vkinit::PipelineBuilder()
          .set_device(m_Device)
          .set_render_pass(m_RenderPass)
          .set_pipeline_layout(m_MeshPipelineLayout)
          .set_vertex_description(vertexDescription.attributes,
                                  vertexDescription.bindings)
          .add_shader_module(triangleVertexShader, utils::ShaderType::Vertex)
          .add_shader_module(triangleFragShader, utils::ShaderType::Fragment)
          .set_viewport({0, 0}, m_WindowExtent, {0.0f, 1.0f})
          //.set_scissor({ 0, 0 }, { m_WindowExtent.width / 2,
          // m_WindowExtent.height })
          .set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
          .build()
          .value();

  create_material(m_MeshPipeline, m_MeshPipelineLayout, "defaultmesh");

  /* m_MeshPipeline = pipelineBuilder.build(); */

  vkDestroyShaderModule(m_Device, triangleFragShader, nullptr);
  vkDestroyShaderModule(m_Device, triangleVertexShader, nullptr);

  m_MainDeletionQueue.push_function([=]() {
    vkDestroyPipeline(m_Device, m_MeshPipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_TrianglePipelineLayout, nullptr);
    vkDestroyPipelineLayout(m_Device, m_MeshPipelineLayout, nullptr);
  });
}

void VulkanEngine::load_meshes() {
  m_MonkeyMesh = load_mesh_from_obj("res/models/monkey_smooth.obj").value();

  m_TriangleMesh.vertices.resize(3);
  m_TriangleMesh.vertices[0].position = {0.5f, 0.5f, 0.0f};
  m_TriangleMesh.vertices[1].position = {-0.5f, 0.5f, 0.0f};
  m_TriangleMesh.vertices[2].position = {0.f, -0.5f, 0.0f};

  m_TriangleMesh.vertices[0].color = {1.f, 0.f, 0.0f};
  m_TriangleMesh.vertices[1].color = {0.f, 1.f, 0.0f};
  m_TriangleMesh.vertices[2].color = {0.f, 0.f, 1.0f};

  upload_mesh(m_MonkeyMesh);
  upload_mesh(m_TriangleMesh);

  m_Meshes["monkey"] = m_MonkeyMesh;
  m_Meshes["triangle"] = m_TriangleMesh;
}

void VulkanEngine::upload_mesh(Mesh &mesh) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo,
                           &mesh.vertexBuffer.buffer,
                           &mesh.vertexBuffer.allocation, nullptr));

  m_MainDeletionQueue.push_function([=]() {
    vmaDestroyBuffer(m_Allocator, mesh.vertexBuffer.buffer,
                     mesh.vertexBuffer.allocation);
  });

  void *data;
  vmaMapMemory(m_Allocator, mesh.vertexBuffer.allocation, &data);
  memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(m_Allocator, mesh.vertexBuffer.allocation);
}

void VulkanEngine::init_scene() {
  RenderObject monkey;
  monkey.mesh = get_mesh("monkey");
  monkey.material = get_material("defaultmesh");
  monkey.transformMatrix = glm::mat4(1.0f);

  m_RenderObjects.push_back(monkey);

  for (int x = -20; x <= 20; x++) {
    for (int y = -20; y <= 20; y++) {

      RenderObject tri{};
      tri.mesh = get_mesh("triangle");
      tri.material = get_material("defaultmesh");
      glm::mat4 translation =
          glm::translate(glm::mat4{1.0}, glm::vec3(x, 0, y));
      glm::mat4 scale = glm::scale(glm::mat4{1.0}, glm::vec3(0.2, 0.2, 0.2));
      tri.transformMatrix = translation * scale;

      m_RenderObjects.push_back(tri);
    }
  }
}

void VulkanEngine::init_descriptors() {

  std::vector<VkDescriptorPoolSize> sizes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};

  // create a descriptor pool that will hold 10 uniform buffers
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = 0;
  poolInfo.maxSets = 10;
  poolInfo.poolSizeCount = (uint32_t)sizes.size();
  poolInfo.pPoolSizes = sizes.data();

  vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);

  VkDescriptorSetLayoutBinding cameraBind =
      vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           VK_SHADER_STAGE_VERTEX_BIT, 0);
  VkDescriptorSetLayoutBinding sceneBind = vkinit::descriptorset_layout_binding(
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

  VkDescriptorSetLayoutBinding bindings[] = {cameraBind, sceneBind};

  /* VkDescriptorSetLayoutBinding camBufferBinding{}; */
  /* camBufferBinding.binding = 0; */
  /* camBufferBinding.descriptorCount = 1; */
  /* camBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; */
  /* camBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; */

  VkDescriptorSetLayoutCreateInfo setInfo{};
  setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  setInfo.pNext = nullptr;
  setInfo.bindingCount = 2;
  setInfo.flags = 0;
  setInfo.pBindings = bindings;

  vkCreateDescriptorSetLayout(m_Device, &setInfo, nullptr, &m_GlobalSetLayout);

  VkDescriptorSetLayoutBinding objectBind =
      vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                           VK_SHADER_STAGE_VERTEX_BIT, 0);

  VkDescriptorSetLayoutCreateInfo set2Info{};
  set2Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set2Info.pNext = nullptr;
  set2Info.bindingCount = 1;
  set2Info.flags = 0;
  set2Info.pBindings = &objectBind;

  vkCreateDescriptorSetLayout(m_Device, &set2Info, nullptr, &m_ObjectSetLayout);

  const size_t sceneParamBufferSize =
      FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));

  m_SceneParameterBuffer =
      create_buffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < FRAME_OVERLAP; i++) {

    const int MAX_OBJECTS = 10000; // TODO: dynamic object buffer?

    m_Frames[i].objectBuffer = create_buffer(
        sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);

    m_Frames[i].cameraBuffer =
        create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_GlobalSetLayout;

    vkAllocateDescriptorSets(m_Device, &allocInfo,
                             &m_Frames[i].globalDescriptor);

    VkDescriptorSetAllocateInfo objectAlloc{};
    objectAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    objectAlloc.pNext = nullptr;
    objectAlloc.descriptorPool = m_DescriptorPool;
    objectAlloc.descriptorSetCount = 1;
    objectAlloc.pSetLayouts = &m_ObjectSetLayout;

    vkAllocateDescriptorSets(m_Device, &objectAlloc,
                             &m_Frames[i].objectDescriptor);

    VkDescriptorBufferInfo cameraInfo;
    cameraInfo.buffer = m_Frames[i].cameraBuffer.buffer;
    cameraInfo.offset = 0;
    cameraInfo.range = sizeof(GPUCameraData);

    VkDescriptorBufferInfo sceneInfo;
    sceneInfo.buffer = m_SceneParameterBuffer.buffer;
    sceneInfo.offset = 0;
    sceneInfo.range = sizeof(GPUSceneData);

    VkDescriptorBufferInfo objectBufferInfo;
    objectBufferInfo.buffer = m_Frames[i].objectBuffer.buffer;
    objectBufferInfo.offset = 0;
    objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

    VkWriteDescriptorSet cameraWrite = vkinit::write_descriptor_buffer(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Frames[i].globalDescriptor,
        &cameraInfo, 0);

    VkWriteDescriptorSet sceneWrite = vkinit::write_descriptor_buffer(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_Frames[i].globalDescriptor,
        &sceneInfo, 1);

    VkWriteDescriptorSet objectWrite = vkinit::write_descriptor_buffer(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_Frames[i].objectDescriptor,
        &objectBufferInfo, 0);

    VkWriteDescriptorSet setWrites[] = {cameraWrite, sceneWrite, objectWrite};

    /* VkDescriptorBufferInfo bufferInfo{}; */
    /* bufferInfo.buffer = m_Frames[i].cameraBuffer.buffer; */
    /* bufferInfo.offset = 0; */
    /* bufferInfo.range = sizeof(GPUCameraData); */

    /* VkWriteDescriptorSet setWrite{}; */
    /* setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; */
    /* setWrite.pNext = nullptr; */
    /* setWrite.dstBinding = 0; */
    /* setWrite.dstSet = m_Frames[i].globalDescriptor; */
    /* setWrite.descriptorCount = 1; */
    /* setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; */
    /* setWrite.pBufferInfo = &bufferInfo; */

    vkUpdateDescriptorSets(m_Device, 3, setWrites, 0, nullptr);
  }

  m_MainDeletionQueue.push_function([&]() {
    vmaDestroyBuffer(m_Allocator, m_SceneParameterBuffer.buffer,
                     m_SceneParameterBuffer.allocation);

    vkDestroyDescriptorSetLayout(m_Device, m_GlobalSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_ObjectSetLayout, nullptr);

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
      vmaDestroyBuffer(m_Allocator, m_Frames[i].objectBuffer.buffer,
                       m_Frames[i].objectBuffer.allocation);
      vmaDestroyBuffer(m_Allocator, m_Frames[i].cameraBuffer.buffer,
                       m_Frames[i].cameraBuffer.allocation);
    }
  });
}

FrameData &VulkanEngine::get_current_frame() {
  return m_Frames[m_FrameNumber % FRAME_OVERLAP];
}
AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize,
                                            VkBufferUsageFlags usage,
                                            VmaMemoryUsage memoryUsage) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;

  bufferInfo.size = allocSize;
  bufferInfo.usage = usage;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = memoryUsage;

  AllocatedBuffer newBuffer;

  VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo,
                           &newBuffer.buffer, &newBuffer.allocation, nullptr));

  return newBuffer;
}
size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize) {
  size_t minUboAlignment =
      m_GPUProperties.limits.minUniformBufferOffsetAlignment;
  size_t alignedSize = originalSize;
  if (minUboAlignment > 0) {
    alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
  }
  return alignedSize;
}
