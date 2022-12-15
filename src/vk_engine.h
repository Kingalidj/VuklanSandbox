#pragma once

#include "vk_types.h"
#include "vk_descriptors.h"
#include "vk_manager.h"
#include "event.h"

#include <glm/glm.hpp>

class Window;

namespace vkutil {

	void full_pipeline_barrier(VkCommandBuffer cmd);

	//struct GPUCameraData {
	//	glm::mat4 view;
	//	glm::mat4 proj;
	//	glm::mat4 viewProj;
	//};

	//struct GPUObjectData {
	//	glm::mat4 modelMatrix;
	//};

	void memory_barrier(VkCommandBuffer cmd, VkAccessFlagBits srcAccessBit, VkAccessFlagBits dstAccessBit, VkPipelineStageFlagBits srcStageBit, VkPipelineStageFlagBits dstStageBit);

	struct FrameData {
		VkSemaphore presentSemaphore, renderSemaphore;
		VkFence renderFence;

		VkCommandPool commandPool;
		VkCommandBuffer renderCommandBuffer;

		VkCommandBuffer activeCommandBuffer{ VK_NULL_HANDLE };

		AllocatedBuffer cameraBuffer;
		AllocatedBuffer objectBuffer;

		VkDescriptorSet cameraDescriptor{ VK_NULL_HANDLE };
		VkDescriptorSet objectDescriptor{ VK_NULL_HANDLE };
	};

	struct DynRenderpassInfo {
		VkImage boundImage{ VK_NULL_HANDLE };
		bool active{ false };
	};

	class  VulkanEngine {
	public:
		float m_RenderResolution = 1.0f;

		VulkanEngine(Window &window);

		//void draw();
		void cleanup();

		void resize_window(uint32_t w, uint32_t h);
		void resize_viewport(uint32_t w, uint32_t h);

		void prepare_frame(uint32_t *swapchainImageIndex);
		void end_frame(uint32_t swapchainImageIndex);

		void exec_renderpass(VkRenderPass renderpass, VkFramebuffer framebuffer, uint32_t w, uint32_t h,
			uint32_t attachmentCount, glm::vec4 clearColor, std::function<void()> &&func);
		void exec_swapchain_renderpass(uint32_t swapchainImageIndex, glm::vec4 color, std::function<void()> &&func);

		void dyn_renderpass(VkTexture &color, VkTexture &depth, glm::vec4 clearColor, std::function<void()> &&func);
		void dyn_renderpass(VkTexture &color, glm::vec4 clearColor, std::function<void()> &&func);

		void begin_renderpass(VkTexture &color, VkTexture &depth, glm::vec4 clearColor);
		void begin_renderpass(VkTexture &color, glm::vec4 clearColor);
		void end_renderpass();

		//void draw_objects(VkCommandBuffer cmd, RenderObject *first, uint32_t count);
		size_t pad_uniform_buffer_size(size_t originalSize);

		VulkanManager &manager();
		AssetManager &asset_manager();

		VkFormat get_color_format();
		VkFormat get_depth_format();
		VkInstance get_instance();
		VkPhysicalDevice get_physical_device();
		VkQueue get_graphics_queue();
		uint32_t get_queue_family_index();
		VkRenderPass get_swapchain_renderpass();
		const VkDevice device();
		VkCommandBuffer get_active_command_buffer();

		void wait_idle();

		PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;

	private:
		void init_vulkan(Window &window);
		void init_commands();
		void init_sync_structures();
		//void init_pipelines();
		//void init_scene();
		//void init_descriptors();
		void init_imgui(Window &window);

		void init_swapchain();
		void init_renderpass();
		void init_framebuffers();
		void cleanup_swapchain();
		void rebuild_swapchain();

		void init_vp_framebuffers();
		void rebuild_vp_framebuffer();

		//void load_meshes();
		//void load_images();
		//void upload_mesh(Ref<Mesh> mesh);

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
		VkRenderPass m_RenderPass;

		VkFormat m_SwapchainImageFormat;
		VkFormat m_DepthFormat;

		FrameData m_FrameData;
		DynRenderpassInfo m_DynRenderpassInfo;

		VkTexture m_ColorTexture;
		VkTexture m_DepthTexture;

		DeletionQueue m_MainDeletionQueue;

		VulkanManager m_VkManager;
		AssetManager m_AssetManager;

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
