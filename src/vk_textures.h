#pragma once

#include "vk_types.h"
#include "imgui_impl_vulkan.h"


class Texture {
public:
	AllocatedImage imageBuffer;
	VkImageView imageView;

	uint32_t width, height, nChannels;

	ImTextureID ImGuiTexID;
};

class VulkanEngine;

namespace vkutil {

	std::optional<Ref<Texture>> load_texture(const char *file, VulkanEngine &engine, VkSamplerCreateInfo info);

	bool load_alloc_image_from_file(const char *file, VulkanEngine &engine,
		AllocatedImage *outImage, int *width, int *height, int *nChannels);
}
