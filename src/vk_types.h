#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

struct AllocatedBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
};

struct AllocatedImage {
	VkImage image;
	VmaAllocation allocation;
};
