#pragma once

#include "vk_types.h"
#include "vk_mesh.h"

#include <glm/glm.hpp>

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 renderMatrix;
};

struct DeletionQueue {

	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& func) { deletors.push_back(func); }

	void flush() {
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)();
		}

		deletors.clear();
	}
};

class VulkanEngine {
public:
	bool m_IsInitialized{ false };
	int m_FrameNumber{ 0 };

	VkExtent2D m_WindowExtent{ 800, 600 };

	struct GLFWwindow* m_Window = nullptr;

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
	VkPipelineLayout m_MeshPipelineLayout;
	VkPipeline m_MeshPipeline;

	DeletionQueue m_MainDeletionQueue;

	Mesh m_TriangleMesh;

	/* 	VkBuffer m_VertexBuffer; */
	/* 	VkDeviceMemory m_VertexBufferMemory; */

	VmaAllocator m_Allocator;

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_default_renderpass();
	void init_framebuffers();
	void init_sync_structures();
	void init_pipelines();

	void load_meshes();
	void upload_mesh(Mesh& mesh);
};
