#pragma once

#include "vk_scene.h"
#include "vk_types.h"
#include "vk_descriptors.h"
#include "vk_textures.h"
#include "vk_manager.h"

#include "window.h"

#include <glm/glm.hpp>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

struct GPUSceneData {
	//glm::vec4 fogColor;
	//glm::vec4 fogDistance;
	//glm::vec4 ambientColor;
	//glm::vec4 sunlightDirection;
	//glm::vec4 sunlightColor;
};

struct GPUCameraData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewProj;
};

struct GPUObjectData {
	glm::mat4 modelMatrix;
};

struct FrameData {
	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	AllocatedBuffer cameraBuffer;
	AllocatedBuffer objectBuffer;

	VkDescriptorSet globalDescriptor;
	VkDescriptorSet objectDescriptor;
};

/* struct MeshPushConstants { */
/*   glm::vec4 data; */
/*   glm::mat4 renderMatrix; */
/* }; */
//constexpr uint32_t FRAME_OVERLAP = 2;

class  VulkanEngine {
public:
	bool m_IsInitialized{ false };
	int m_FrameNumber{ 0 };

	VkExtent2D m_WindowExtent{ 1600, 900 };
	VkExtent2D m_ViewportExtent{ 1600, 900 };

	//struct GLFWwindow *m_Window = nullptr;

	Window *m_Window = nullptr;

	void init();
	void draw();
	void run();
	void cleanup();

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage,
		VmaMemoryUsage memoryUsage);

	void draw_objects(VkCommandBuffer cmd, RenderObject *first, int count);

	size_t pad_uniform_buffer_size(size_t originalSize);

	//void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function);

	//void upload_to_gpu(void *data, uint32_t size, AllocatedBuffer &buffer, VkBufferUsageFlags flags);



	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface;
	VkPhysicalDeviceProperties m_GPUProperties;

	VkSwapchainKHR m_Swapchain;
	VkFormat m_SwapchainImageFormat;
	VkFormat m_DepthFormat;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily;

	FrameData m_FrameData;
	//FrameData &get_current_frame();

	VkRenderPass m_ImGuiRenderPass;
	VkRenderPass m_ViewportRenderPass;

	std::vector<Texture> m_ViewportTextures;
	std::vector<VkFramebuffer> m_ViewportFrameBuffers;

	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;

	std::vector<VkFramebuffer> m_ImGuiFrameBuffers;

	/* VkSemaphore m_PresentSemaphore, m_RenderSemaphore; */
	/* VkFence m_RenderFence; */

	VkPipelineLayout m_TrianglePipelineLayout;
	VkPipelineLayout m_MeshPipelineLayout;
	VkPipeline m_MeshPipeline;

	DeletionQueue m_MainDeletionQueue;

	VkImageView m_DepthImageView;
	AllocatedImage m_DepthImage;


	std::vector<RenderObject> m_RenderObjects;

	VulkanManager m_VkManager;

	VmaAllocator m_Allocator;

	VkDescriptorSetLayout m_GlobalSetLayout;
	VkDescriptorSetLayout m_ObjectSetLayout;
	VkDescriptorSetLayout m_SingleTextureSetLayout;

	VkDescriptorPool m_DescriptorPool;

	GPUSceneData m_SceneParameters;
	AllocatedBuffer m_SceneParameterBuffer;

	vkutil::DescriptorAllocator m_DescriptorAllocator;
	vkutil::DescriptorLayoutCache m_DescriptorLayoutCache;

	bool m_FramebufferResized = false;
	bool m_ViewportbufferResized = false;
	bool m_WindowMinimized = false;

private:
	void init_vulkan();
	void init_commands();
	void init_sync_structures();
	void init_pipelines();
	void init_scene();
	void init_descriptors();
	void init_imgui();

	void init_swapchain();
	void init_renderpass();
	void init_framebuffers();
	void cleanup_swapchain();
	void rebuild_swapchain();

	void init_vp_swapchain();
	void init_vp_renderpass();
	void init_vp_framebuffers();
	void cleanup_vp_swapchain();
	void rebuild_vp_swapchain();

	void load_meshes();
	void load_images();
	void upload_mesh(Ref<Mesh> mesh);

	void on_event(Atlas::Event &e);
	bool on_window_resize(Atlas::WindowResizedEvent &e);
	bool on_viewport_resize(Atlas::ViewportResizedEvent &e);
};
