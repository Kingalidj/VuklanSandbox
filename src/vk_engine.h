#pragma once

#include "vk_scene.h"
#include "vk_types.h"
#include "vk_descriptors.h"
#include "vk_textures.h"
#include "vk_manager.h"
#include "vk_framebuffer.h"
#include "vk_buffer.h"

#include "window.h"

#include <glm/glm.hpp>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace vkutil {

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

		VkDescriptorSet cameraDescriptor;
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
		float m_RenderResolution = 1.0f;

		//struct GLFWwindow *m_Window = nullptr;

		Scope<Window> m_Window = nullptr;

		void init();
		uint32_t prepare_frame();
		void draw();
		void run();
		void cleanup();

		void draw_objects(VkCommandBuffer cmd, RenderObject *first, int count);

		size_t pad_uniform_buffer_size(size_t originalSize);

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

		VkRenderPass m_ImGuiRenderPass;
		VkRenderPass m_ViewportRenderPass;

		Framebuffer m_ViewportFramebuffer;

		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		std::vector<VkFramebuffer> m_ImGuiFrameBuffers;

		DeletionQueue m_MainDeletionQueue;

		std::vector<RenderObject> m_RenderObjects;

		VulkanManager m_VkManager;

		VmaAllocator m_Allocator;

		VkDescriptorSetLayout m_CameraSetLayout;
		VkDescriptorSetLayout m_ObjectSetLayout;
		VkDescriptorSetLayout m_SingleTextureSetLayout;

		bool m_FramebufferResized = false;
		bool m_ViewportbufferResized = false;

		bool m_WindowMinimized = false;
		bool m_ViewportMinimized = false;

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

		void init_vp_renderpass();
		void init_vp_framebuffers();
		void rebuild_vp_swapchain();

		void load_meshes();
		void load_images();
		void upload_mesh(Ref<Mesh> mesh);

		void on_event(Atlas::Event &e);
		bool on_window_resize(Atlas::WindowResizedEvent &e);
		bool on_viewport_resize(Atlas::ViewportResizedEvent &e);
	};

}
