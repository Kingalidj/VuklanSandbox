#include "vk_engine.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "vk_initializers.h"
#include "vk_types.h"

#include "VkBootstrap.h"

#include <iostream>

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      std::cout << "at line: " << __LINE__ << ", in file: " << __FILE__        \
                << "\n";                                                       \
      std::cout << "Detected Vulkan error: " << err << std::endl;              \
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

  m_IsInitialized = true;
}

void VulkanEngine::cleanup() {
  if (m_IsInitialized) {
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);

		for (int i = 0; i < m_SwapChainImageViews.size(); ++i) {
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

void VulkanEngine::draw() {}

void VulkanEngine::run() {

  while (!glfwWindowShouldClose(m_Window)) {
    glfwPollEvents();

    draw();
  }
}

void VulkanEngine::init_vulkan() {
  vkb::InstanceBuilder builder;

  vkb::Instance vkb_inst = builder.set_app_name("Vulkan Application")
                               .request_validation_layers(true)
                               .require_api_version(1, 1, 0)
                               .use_default_debug_messenger()
                               .build()
                               .value();

  m_Instance = vkb_inst.instance;
  m_DebugMessenger = vkb_inst.debug_messenger;

  VK_CHECK(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface));

  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 1)
                                           .set_surface(m_Surface)
                                           .select()
                                           .value();

  vkb::DeviceBuilder deviceBuilder{physicalDevice};
  vkb::Device vkbDevice = deviceBuilder.build().value();

  m_Device = vkbDevice.device;
  m_ChosenGPU = physicalDevice.physical_device;

	m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::init_swapchain() {
  vkb::SwapchainBuilder swapchainBuilder{m_ChosenGPU, m_Device, m_Surface};

  vkb::Swapchain vkbSwapchain =
      swapchainBuilder.use_default_format_selection()
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
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_CommandPool));

	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_CommandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_MainCommandBuffer));
}
