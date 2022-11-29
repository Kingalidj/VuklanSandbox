#pragma once

#include "vk_types.h"
#include "vk_textures.h"
#include "vk_buffer.h"
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

		const VkDevice device() const;
		const VmaAllocator get_allocator() const;
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

	class AssetManager {
	public:

		void cleanup(VulkanManager &manager) {

			for (auto &shader : m_Shaders) vkDestroyPipeline(manager.device(), shader->pipeline, nullptr);
			for (auto &texture : m_Textures) destroy_texture(manager, *texture.get());
			for (auto &buffer : m_Buffers) destroy_buffer(manager, *buffer.get());

			m_Shaders.clear();
			m_Textures.clear();
			m_Buffers.clear();
		}

		template<typename ...Args>
		WeakRef<Texture> register_texture(Args &&...args) {
			Ref<Texture> texture = make_ref<Texture>(std::forward<Args>(args)...);
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

		void deregister_buffer(Ref<AllocatedBuffer> &buffer) {
			auto it = m_Buffers.find(buffer);

			if (it == m_Buffers.end()) {
				CORE_WARN("Buffer was never registered: {}", buffer);
			}

			m_Buffers.erase(it);
		}

		void deregister_shader(Ref<Shader> &shader) {
			auto it = m_Shaders.find(shader);

			if (it == m_Shaders.end()) {
				CORE_WARN("Shader was never registered: {}", shader);
			}

			m_Shaders.erase(it);
		}

		void deregister_texture(Ref<Texture> &texture) {
			auto it = m_Textures.find(texture);

			if (it == m_Textures.end()) {
				CORE_WARN("Texture was never registered: {}", texture);
			}

			m_Textures.erase(it);
		}

	private:
		std::unordered_set<Ref<Shader>> m_Shaders;
		std::unordered_set<Ref<Texture>> m_Textures;
		std::unordered_set<Ref<AllocatedBuffer>> m_Buffers;
	};

}
