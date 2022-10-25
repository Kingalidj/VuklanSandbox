#pragma once

#include "vk_types.h"

struct Texture {
	AllocatedImage imageBuffer;
	VkImageView imageView;

	uint32_t width, height, nChannels;

	VkSampler sampler;
	VkDescriptorSet descriptorSet;
};

class VulkanEngine;

namespace vkutil {

	std::optional<Texture> load_const_texture(const char *file, VulkanEngine &engine);

	bool load_alloc_image_from_file(const char *file, VulkanEngine &engine,
		AllocatedImage *outImage, int *width, int *height, int *nChannels);
}
