#include "vk_types.h"
#include "vk_manager.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace vkutil {

	void map_memory(VmaAllocator allocator, AllocatedBuffer *buffer, std::function<void(void *data)> func)
	{
		void *data;

		vmaMapMemory(allocator, buffer->allocation, &data);
		func(data);
		vmaUnmapMemory(allocator, buffer->allocation);
	}

	void destroy_buffer(VulkanManager &manager, AllocatedBuffer &buffer)
	{
		vmaDestroyBuffer(manager.get_allocator(), buffer.buffer, buffer.allocation);
	}

}
