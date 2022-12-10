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

	void insert_buffer_memory_barrier(VkCommandBuffer command_buffer, AllocatedBuffer &buffer, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
		uint32_t srcQueueFamilyIndx, uint32_t dstQueueFamilyIndx,
		VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask)
	{
		VkBufferMemoryBarrier barrier{};
		//VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = src_access_mask;
		barrier.dstAccessMask = dst_access_mask;
		barrier.buffer = buffer.buffer;

		barrier.srcQueueFamilyIndex = srcQueueFamilyIndx;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndx;

		vkCmdPipelineBarrier(
			command_buffer,
			src_stage_mask,
			dst_stage_mask,
			0,
			0, nullptr,
			1, &barrier,
			0, nullptr);
	}

}