#pragma once

#include "vk_types.h"
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

		const VkDevice device() const;
		const VmaAllocator get_allocator() const;
		DescriptorAllocator &get_descriptor_allocator();
		DescriptorLayoutCache &get_descriptor_layout_cache();

		PipelineLayoutCache &get_pipeline_layout_cache();

		void init(VkDevice device, VmaAllocator allocator);
		void init_commands(VkQueue queue, uint32_t queueFamilyIndex);
		void init_sync_structures();

		void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&func);

		void cleanup();

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
	};

	class AssetManager {
	public:

		void cleanup(VulkanManager &manager);
		void destroy_queued(VulkanManager &manager);

		template<typename ...Args>
		WeakRef<VkTexture> register_texture(Args &&...args) {
			Ref<VkTexture> texture = make_ref<VkTexture>(std::forward<Args>(args)...);
			m_Textures.insert(texture);

			return texture;
		}

		template<typename ...Args>
		WeakRef<Shader> register_shader(Args &&...args) {
			Ref<Shader> shader = make_ref<Shader>(std::forward<Args>(args)...);
			m_Shaders.insert(shader);

			return shader;
		}

		template<typename ...Args>
		WeakRef<AllocatedBuffer> register_buffer(Args &&...args) {
			Ref<AllocatedBuffer> buffer = make_ref<AllocatedBuffer>(std::forward<Args>(args)...);
			m_Buffers.insert(buffer);

			return buffer;
		}

		void queue_destory_buffer(Ref<AllocatedBuffer> &buffer);
		void deregister_buffer(Ref<AllocatedBuffer> &buffer);
		void queue_destroy_shader(Ref<Shader> &shader);
		void deregister_shader(Ref<Shader> &shader);
		void queue_destroy_texture(Ref<VkTexture> &texture);
		void deregister_texture(Ref<VkTexture> &texture);

	private:
		std::unordered_set<Ref<Shader>> m_Shaders;
		std::unordered_set<Ref<VkTexture>> m_Textures;
		std::unordered_set<Ref<AllocatedBuffer>> m_Buffers;

		std::unordered_set<Ref<Shader>> m_DeletedShaders;
		std::unordered_set<Ref<VkTexture>> m_DeletedTextures;
		std::unordered_set<Ref<AllocatedBuffer>> m_DeletedBuffers;
	};

}
