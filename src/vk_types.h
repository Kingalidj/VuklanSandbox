#pragma once

#define VMA_VULKAN_VERSION 1002000
#include <vk_mem_alloc.h>

#include <vulkan/vulkan_core.h>

namespace vkutil {

	struct VulkanManager;

	struct AllocatedBuffer {
		VkBuffer buffer;
		VmaAllocation allocation;

	};

	void map_memory(VmaAllocator allocator, AllocatedBuffer *buffer, std::function<void(void *data)> func);

	template <typename T>
	void map_memory(VmaAllocator allocator, AllocatedBuffer *buffer, T *memory, uint32_t size) {
		void *data;

		vmaMapMemory(allocator, buffer->allocation, &data);
		memcpy(data, memory, size);
		vmaUnmapMemory(allocator, buffer->allocation);
	}

	void destroy_buffer(VulkanManager &manager, AllocatedBuffer &buffer);

	struct AllocatedImage {
		VkImage image;
		VmaAllocation allocation;
	};

}
