#pragma once

#include "vk_types.h"

#include <vector>

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
	VkSurfaceKHR m_Surface; //vulkan window

	VkSwapchainKHR m_SwapChain;
	VkFormat m_SwapChainImageFormat;

	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily;

	VkCommandPool m_CommandPool;
	VkCommandBuffer m_MainCommandBuffer;

	VkRenderPass m_RenderPass;
	std::vector<VkFramebuffer> m_FrameBuffers;

	VkSemaphore m_PresentSemaphore, m_RenderSemaphore;
	VkFence m_RenderFence;

private:

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_default_renderpass();
	void init_framebuffers();
	void init_sync_structures();
};
