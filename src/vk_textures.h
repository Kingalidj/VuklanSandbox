#pragma once

#include "vk_types.h"
#include "imgui_impl_vulkan.h"


struct Texture {

	AllocatedImage imageAllocation;
	VkImageView imageView;

	uint32_t width, height;
	VkFormat format;

	bool imguiCompatible;
	ImTextureID imGuiTexID;
	VkSampler imGuiSampler;
};

class VulkanManager;

namespace vkutil {

	struct TextureCreateInfo {
		uint32_t width, height;
		VkFormat format;
		VkFilter filter;
		VkImageUsageFlags usageFlags;
		VkImageAspectFlags aspectFlags;
		bool createImguiDescriptor;
	};

	TextureCreateInfo color_texture_create_info(uint32_t w, uint32_t h, VkFormat format);
	TextureCreateInfo depth_texture_create_info(uint32_t w, uint32_t h, VkFormat format);

	void set_texture_data(Texture &tex, const void *data, VulkanManager &manager);

	void destroy_texture(VulkanManager &manager, Texture &tex);

	void alloc_texture(VulkanManager& manager, TextureCreateInfo &info, Texture *tex);

	std::optional<Ref<Texture>> load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info);

	bool load_alloc_image_from_file(const char *file, VulkanManager &manager,
		AllocatedImage *outImage, int *width, int *height, int *nChannels);
}
