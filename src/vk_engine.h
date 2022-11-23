#pragma once

#include "vk_scene.h"
#include "vk_types.h"
#include "vk_descriptors.h"
#include "vk_textures.h"
#include "vk_manager.h"
#include "vk_buffer.h"
#include "event.h"

#include <glm/glm.hpp>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

class Window;

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

	class  VulkanEngine {
	public:
		float m_RenderResolution = 1.0f;

		VulkanEngine(Window &window);

		void draw();
		void cleanup();

		void resize_window(uint32_t w, uint32_t h);
		void resize_viewport(uint32_t w, uint32_t h);

		void prepare_frame(uint32_t *swapchainImageIndex);
		void end_frame(uint32_t swapchainImageIndex);

		void exec_renderpass(VkRenderPass renderpass, VkFramebuffer framebuffer, uint32_t w, uint32_t h,
			uint32_t attachmentCount, glm::vec4 clearColor, std::function<void()> &&func);

		void dyn_renderpass(Texture &color, Texture &depth, glm::vec4 clearColor, std::function<void()> &&func);
		void dyn_renderpass(Texture &color, glm::vec4 clearColor, std::function<void()> &&func);

		void draw_objects(VkCommandBuffer cmd, RenderObject *first, uint32_t count);
		size_t pad_uniform_buffer_size(size_t originalSize);

	private:
		void init_vulkan(Window &window);
		void init_commands();
		void init_sync_structures();
		void init_pipelines();
		void init_scene();
		void init_descriptors();
		void init_imgui(Window &window);

		void init_swapchain();
		void init_renderpass();
		void init_framebuffers();
		void cleanup_swapchain();
		void rebuild_swapchain();

		void init_vp_framebuffers();
		void rebuild_vp_framebuffer();

		void load_meshes();
		void load_images();
		void upload_mesh(Ref<Mesh> mesh);

		bool m_IsInitialized{ false };
		int m_FrameNumber{ 0 };

		VkExtent2D m_WindowExtent{ 1600, 900 };
		VkExtent2D m_ViewportExtent{ 1600, 900 };

		using EventCallbackFn = std::function<void(Atlas::Event &)>;
		const EventCallbackFn &m_EventCallback;

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;
		VkPhysicalDeviceProperties m_GPUProperties;
		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueFamily;

		VkSwapchainKHR m_Swapchain;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		VkFormat m_SwapchainImageFormat;
		VkFormat m_DepthFormat;

		FrameData m_FrameData;

		VkRenderPass m_ImGuiRenderPass;

		Texture m_ColorTexture;
		Texture m_DepthTexture;

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


	};

}
