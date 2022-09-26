#pragma once

#include "vk_types.h"

class VulkanEngine {
public:

  bool m_IsInitialized{false};
  int m_FrameNumber{0};

  VkExtent2D m_WindowExtent{1700, 900};

  struct GLFWwindow *m_Window = nullptr;

  void init();
  void cleanup();
  void draw();
  void run();

	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkPhysicalDevice m_ChosenGPU;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface; //vulkan window

private:

	void init_vulkan();
};
