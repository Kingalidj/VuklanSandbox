#pragma once

#include "vk_types.h"

struct Texture {
	AllocatedImage imageBuffer;
	VkImageView imageView;
};

class VulkanEngine;

namespace vkutil {

bool load_image_from_file(const char *file, VulkanEngine &engine,
                          AllocatedImage &outImage);
}
