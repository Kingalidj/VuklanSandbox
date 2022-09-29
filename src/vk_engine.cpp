#include "vk_engine.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "vk_initializers.h"
#include "vk_types.h"

#include "VkBootstrap.h"

#include <cmath>
#include <iostream>

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      std::cerr << "at line: " << __LINE__ << ", in file: " << __FILE__        \
                << "\n";                                                       \
      std::cerr << "Detected Vulkan error: " << err << std::endl;              \
      abort();                                                                 \
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

  m_IsInitialized = true;
}

void VulkanEngine::cleanup() {
  if (m_IsInitialized) {

    vkDeviceWaitIdle(m_Device);

    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

    vkDestroyFence(m_Device, m_RenderFence, nullptr);
    vkDestroySemaphore(m_Device, m_RenderSemaphore, nullptr);
    vkDestroySemaphore(m_Device, m_PresentSemaphore, nullptr);

    vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);

    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

    for (int i = 0; i < m_FrameBuffers.size(); i++) {
      vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
      vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);
    }

    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
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
  VK_CHECK(vkAcquireNextImageKHR(m_Device, m_SwapChain, 1000000000,
                                 m_PresentSemaphore, nullptr,
                                 &swapchainImageIndex));

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.pNext = nullptr;

  cmdBeginInfo.pInheritanceInfo = nullptr;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(m_MainCommandBuffer, &cmdBeginInfo));

  VkClearValue clearValue;
  float flash = std::abs(std::sin(m_FrameNumber / 120.f));
  clearValue.color = {{0.0f, 0.0f, flash, 1.0f}};

  // start main renderpass
  VkRenderPassBeginInfo rpInfo = {};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpInfo.pNext = nullptr;

  rpInfo.renderPass = m_RenderPass;
  rpInfo.renderArea.offset.x = 0;
  rpInfo.renderArea.offset.y = 0;
  rpInfo.renderArea.extent = m_WindowExtent;
  rpInfo.framebuffer = m_FrameBuffers[swapchainImageIndex];

  rpInfo.clearValueCount = 1;
  rpInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(m_MainCommandBuffer, &rpInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdEndRenderPass(m_MainCommandBuffer);
  VK_CHECK(vkEndCommandBuffer(m_MainCommandBuffer));

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.pNext = nullptr;

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  submit.pWaitDstStageMask = &waitStage;

  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &m_PresentSemaphore;

  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &m_RenderSemaphore;

  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &m_MainCommandBuffer;

  VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submit, m_RenderFence));

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;

  presentInfo.pSwapchains = &m_SwapChain;
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

void VulkanEngine::init_vulkan() {
  vkb::Instance vkb_inst =
      vkb::InstanceBuilder()
          .set_app_name("Vulkan Application")
          .request_validation_layers(true)
          .require_api_version(1, 1, 0)
          .use_default_debug_messenger()
          .set_debug_messenger_severity(
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
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

  m_SwapChain = vkbSwapchain.swapchain;
  m_SwapChainImages = vkbSwapchain.get_images().value();
  m_SwapChainImageViews = vkbSwapchain.get_image_views().value();

  m_SwapChainImageFormat = vkbSwapchain.image_format;
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
}

void VulkanEngine::init_default_renderpass() {
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = m_SwapChainImageFormat;
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
}

void VulkanEngine::init_framebuffers() {
  VkFramebufferCreateInfo fb_info = {};
  fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fb_info.pNext = nullptr;

  fb_info.renderPass = m_RenderPass;
  fb_info.attachmentCount = 1;
  fb_info.width = m_WindowExtent.width;
  fb_info.height = m_WindowExtent.height;
  fb_info.layers = 1;

  const uint32_t swapchain_imagecount = m_SwapChainImages.size();
  m_FrameBuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

  // create framebuffers for each of the swapchain image views
  for (int i = 0; i < swapchain_imagecount; i++) {
    fb_info.pAttachments = &m_SwapChainImageViews[i];
    VK_CHECK(
        vkCreateFramebuffer(m_Device, &fb_info, nullptr, &m_FrameBuffers[i]));
  }
}

void VulkanEngine::init_sync_structures() {
  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.pNext = nullptr;
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_RenderFence));

  VkSemaphoreCreateInfo semaphoreCreateInfo = {};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreCreateInfo.pNext = nullptr;
  semaphoreCreateInfo.flags = 0;
  VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
                             &m_PresentSemaphore));
  VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
                             &m_RenderSemaphore));
}
