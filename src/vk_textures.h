#pragma once

#include "vk_types.h"
#include "imgui_impl_vulkan.h"


struct Texture {

	AllocatedImage imageAllocation;
	VkImageView imageView;

	uint32_t width, height, nChannels;

	ImTextureID ImGuiTexID;
};

class VulkanManager;

namespace vkutil {

	void set_texture_data(Texture &tex, const void *data, VulkanManager &manager);

	void destroy_texture(VulkanManager &manager, Texture &tex);

	void alloc_texture(uint32_t w, uint32_t h, VkSamplerCreateInfo &info, VkFormat format, VulkanManager &manager, Texture *tex, VkImageUsageFlags flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	std::optional<Ref<Texture>> load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info);

	bool load_alloc_image_from_file(const char *file, VulkanManager &manager,
		AllocatedImage *outImage, int *width, int *height, int *nChannels);
}
