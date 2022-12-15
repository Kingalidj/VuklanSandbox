#pragma once

#include "vk_types.h"

namespace vkutil {

	class VulkanManager;

	// --- Buffer util functions ---
	void map_memory(VulkanManager &manager, AllocatedBuffer &buffer, std::function<void(void *data)> func);
	void map_memory(VulkanManager &manager, AllocatedBuffer &buffer, void *memory, uint32_t size);
	void create_buffer(VulkanManager &manager, size_t allocSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, AllocatedBuffer *buffer);
	void upload_to_gpu(VulkanManager &manager, void *copyData, uint32_t size, AllocatedBuffer &buffer, VkBufferUsageFlags flags);
	void staged_upload_to_buffer(VulkanManager &manager, AllocatedBuffer &buffer, void *copyData, uint32_t size);
	void destroy_buffer(VulkanManager &manager, AllocatedBuffer &buffer);

	// --- Image util functions ---
	void create_image(VulkanManager &manager, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags, AllocatedImage *img);


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
	void alloc_texture(VulkanManager &manager, TextureCreateInfo &info, VkTexture *tex);
	void set_texture_data(VulkanManager &manager, VkTexture &tex, void *data);
	//std::optional<Ref<Texture>> load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info);
	bool load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info, VkTexture *tex);
	bool load_alloc_image_from_file(const char *file, VulkanManager &manager,
		AllocatedImage *outImage, int *width, int *height, int *nChannels, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);

	void destroy_texture(const VulkanManager &manager, VkTexture &tex);

}

namespace vkinit {

	VkCommandPoolCreateInfo
		command_pool_create_info(uint32_t queueFamilyIndex,
			VkCommandPoolCreateFlags flags = 0);

	VkCommandBufferAllocateInfo command_buffer_allocate_info(
		VkCommandPool pool, uint32_t count = 1,
		VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkCommandBufferBeginInfo
		command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);

	VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass renderPass,
		VkExtent2D extent);

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

	VkSubmitInfo submit_info(VkCommandBuffer *cmd);

	VkPresentInfoKHR present_info();

	VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass,
		VkExtent2D windowExtent,
		VkFramebuffer framebuffer);

	VkPipelineShaderStageCreateInfo
		pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
			VkShaderModule shaderModule);

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

	VkPipelineInputAssemblyStateCreateInfo
		input_assembly_create_info(VkPrimitiveTopology topology);

	VkPipelineRasterizationStateCreateInfo
		rasterization_state_create_info(VkPolygonMode polygonMode);

	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();

	VkPipelineColorBlendAttachmentState color_blend_attachment_state();

	VkPipelineLayoutCreateInfo pipeline_layout_create_info();

	VkImageCreateInfo image_create_info(VkFormat format,
		VkImageUsageFlags usageFlags,
		VkExtent3D extent);
	VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image,
		VkImageAspectFlags aspectFlags);

	VkPipelineDepthStencilStateCreateInfo
		depth_stencil_create_info(bool depthTest, bool depthWrite,
			VkCompareOp compareOp);

	VkDescriptorSetLayoutBinding
		descriptorset_layout_binding(VkDescriptorType type,
			VkShaderStageFlags stageFlags, uint32_t binding);

	VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type,
		VkDescriptorSet dstSet,
		VkDescriptorBufferInfo *bufferInfo,
		uint32_t binding);

	VkSamplerCreateInfo sampler_create_info(
		VkFilter filters,
		VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type,
		VkDescriptorSet dstSet,
		VkDescriptorImageInfo *imageInfo,
		uint32_t binding);

} // namespace vkinit
