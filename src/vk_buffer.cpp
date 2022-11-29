#include "vk_buffer.h"
#include "vk_manager.h"

namespace vkutil {

	void create_buffer(VulkanManager &manager, size_t allocSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, AllocatedBuffer *buffer)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;

		bufferInfo.size = allocSize;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.requiredFlags = memoryFlags;

		VK_CHECK(vmaCreateBuffer(manager.get_allocator(), &bufferInfo, &vmaAllocInfo,
			&buffer->buffer, &buffer->allocation, nullptr));
	}

	void map_memory(VulkanManager &manager, AllocatedBuffer *buffer, std::function<void(void *data)> func)
	{
		void *data;

		vmaMapMemory(manager.get_allocator(), buffer->allocation, &data);
		func(data);
		vmaUnmapMemory(manager.get_allocator(), buffer->allocation);
	}

	void map_memory(VulkanManager &manager, AllocatedBuffer *buffer, void *memory, uint32_t size)
	{
		map_memory(manager, buffer, [=](void *data) {
			memcpy(data, memory, size);
			});
	}

	void destroy_buffer(VulkanManager &manager, AllocatedBuffer &buffer)
	{
		vmaDestroyBuffer(manager.get_allocator(), buffer.buffer, buffer.allocation);
	}



}