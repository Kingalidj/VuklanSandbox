#pragma once

#include "vk_types.h"

namespace vkutil {

	class VulkanManager;

	void map_memory(VulkanManager &manager, AllocatedBuffer *buffer, std::function<void(void *data)> func);

	void map_memory(VulkanManager &manager, AllocatedBuffer *buffer, void *memory, uint32_t size);
	//template <typename T>
	//void map_memory(VulkanManager &manager, AllocatedBuffer *buffer, T *memory, uint32_t size) {
	//	map_memory(manager, buffer, [=](void *data) {
	//		memcpy(data, memory, size);
	//		});
	//}

	void destroy_buffer(VulkanManager &manager, AllocatedBuffer &buffer);

	void create_buffer(VulkanManager &manager, size_t allocSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, AllocatedBuffer *buffer);

}