#pragma once

#include "vk_types.h"


namespace vkutil {

	class VulkanManager;

	void create_image(VulkanManager &manager, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags, AllocatedImage *img);

	struct Texture {

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

	void insert_image_memory_barrier(
		VkCommandBuffer         command_buffer,
		VkImage                 image,
		VkAccessFlags           src_access_mask,
		VkAccessFlags           dst_access_mask,
		VkImageLayout           old_layout,
		VkImageLayout           new_layout,
		VkPipelineStageFlags    src_stage_mask,
		VkPipelineStageFlags    dst_stage_mask,
		VkImageSubresourceRange range);

	TextureCreateInfo color_texture_create_info(uint32_t w, uint32_t h, VkFormat format);
	TextureCreateInfo depth_texture_create_info(uint32_t w, uint32_t h, VkFormat format);

	void set_texture_data(VulkanManager &manager, Texture &tex, void *data);

	void destroy_texture(const VulkanManager &manager, Texture &tex);

	void alloc_texture(VulkanManager &manager, TextureCreateInfo &info, Texture *tex);

	//std::optional<Ref<Texture>> load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info);
	bool load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info, Texture *tex);

	bool load_alloc_image_from_file(const char *file, VulkanManager &manager,
		AllocatedImage *outImage, int *width, int *height, int *nChannels, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
}
