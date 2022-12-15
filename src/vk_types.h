#pragma once

//#define VMA_VULKAN_VERSION 1001000
#include <vk_mem_alloc.h>

#include <vulkan/vulkan_core.h>

namespace vkutil {

	struct AllocatedImage {
		VkImage image{ VK_NULL_HANDLE };
		VmaAllocation allocation{ VK_NULL_HANDLE };
	};

	struct AllocatedBuffer {
		VkBuffer buffer{ VK_NULL_HANDLE };
		VmaAllocation allocation{ VK_NULL_HANDLE };
	};

	struct VkTexture {

		AllocatedImage imageAllocation;
		VkImageView imageView{ VK_NULL_HANDLE };

		uint32_t width, height;
		VkFormat format;

		bool bImguiDescriptor{ true };
		VkDescriptorSet imguiDescriptor;
		VkSampler sampler;
	};

	struct TextureCreateInfo {
		uint32_t width, height;
		VkFormat format;
		VkFilter filter;
		VkImageUsageFlags usageFlags;
		VkImageAspectFlags aspectFlags;
		bool createImguiDescriptor;
	};
}
