#pragma once

#include "vk_types.h"
#include "vk_textures.h"
#include "vk_scene.h"
#include "vk_descriptors.h"
#include "vk_pipeline.h"

namespace vkutil {

	struct UploadContext {
		VkFence uploadFence;
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
	};

	struct DeletionQueue {

		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()> &&func) { deletors.push_back(func); }

		void flush() {
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)();
			}

			deletors.clear();
		}
	};

	class VulkanManager {
	public:

		VulkanManager() = default;

		const VkDevice get_device();
		const VmaAllocator get_allocator();
		DescriptorAllocator &get_descriptor_allocator();
		DescriptorLayoutCache &get_descriptor_layout_cache();

		PipelineLayoutCache &get_pipeline_layout_cache();

		void init(VkDevice device, VmaAllocator allocator);
		void init_commands(VkQueue queue, uint32_t queueFamilyIndex);
		void init_sync_structures();

		void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&func);
		void upload_to_gpu(void *copyData, uint32_t size, AllocatedBuffer &buffer, VkBufferUsageFlags flags);

		void set_texture(const std::string &name, Ref<Texture> tex);
		void set_mesh(const std::string &name, Ref<Mesh> mesh);
		void set_material(const std::string &name, Ref<Material> material);

		void cleanup();

		std::optional<Ref<Texture>> get_texture(const std::string &name);
		std::optional<Ref<Mesh>> get_mesh(const std::string &name);
		std::optional<Ref<Material>> get_material(const std::string &name);

		std::optional<Ref<Material>> create_material(const std::string &name, VkPipeline pipeline, VkPipelineLayout layout);

		void delete_func(std::function<void()> &&func);


	private:

		VkDevice m_Device{ VK_NULL_HANDLE };
		VmaAllocator m_Allocator{ VK_NULL_HANDLE };

		VkQueue m_Queue{ VK_NULL_HANDLE };
		uint32_t m_QueueFamilyIndex{ 0 };

		UploadContext m_UploadContext{};

		DeletionQueue m_DeletionQueue;

		DescriptorAllocator m_DescriptorAllocator;
		DescriptorLayoutCache m_DescriptorLayoutCache;

		PipelineLayoutCache m_PipelineLayoutCache;

		std::unordered_map<std::string, Ref<Texture>> m_Textures;
		std::unordered_map<std::string, Ref<Mesh>> m_Meshes;
		std::unordered_map<std::string, Ref<Material>> m_Materials;

	};

}
