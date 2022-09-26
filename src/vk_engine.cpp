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

  m_IsInitialized = true;
}

void VulkanEngine::cleanup() {
  if (!m_IsInitialized) {
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
}
