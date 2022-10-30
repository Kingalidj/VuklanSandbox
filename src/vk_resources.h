#pragma once

#include "vk_types.h"
#include "vk_textures.h"
#include "vk_scene.h"

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

class VulkanResourceManager {
public:

	VulkanResourceManager() = default;

	void init(VkDevice device, VmaAllocator allocator);

	void set_texture(const std::string &name, Ref<Texture> tex);
	void set_mesh(const std::string &name, Ref<Mesh> mesh);
	void set_material(const std::string &name, Ref<Material> material);

	void cleanup();

	std::optional<Ref<Texture>> get_texture(const std::string &name);
	std::optional<Ref<Mesh>> get_mesh(const std::string &name);
	std::optional<Ref<Material>> get_material(const std::string &name);

	std::optional<Ref<Material>> create_material(const std::string &name, VkPipeline pipeline, VkPipelineLayout layout);

	void delete_func(std::function<void()> &&func);

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	inline VkDevice get_device();
	inline VmaAllocator get_allocator();

private:

	VkDevice m_Device{ VK_NULL_HANDLE };
	VmaAllocator m_Allocator{ VK_NULL_HANDLE };

	DeletionQueue m_DeletionQueue;

	std::unordered_map<std::string, Ref<Texture>> m_Textures;
	std::unordered_map<std::string, Ref<Mesh>> m_Meshes;
	std::unordered_map<std::string, Ref<Material>> m_Materials;

};
