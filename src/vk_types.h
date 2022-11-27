#pragma once

#define VMA_VULKAN_VERSION 1002000
#include <vk_mem_alloc.h>

#include <vulkan/vulkan_core.h>

namespace vkutil {

	struct AllocatedImage {
		VkImage image{ VK_NULL_HANDLE };
		VmaAllocation allocation;
	};

	struct AllocatedBuffer {
		VkBuffer buffer{ VK_NULL_HANDLE };
		VmaAllocation allocation;
	};
}
