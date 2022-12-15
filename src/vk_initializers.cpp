#include "vk_initializers.h"

#include "vk_manager.h"

#include "imgui_impl_vulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vkutil {

	void create_buffer(VulkanManager &manager, size_t allocSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, AllocatedBuffer *buffer)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;

		bufferInfo.size = allocSize;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.requiredFlags = memoryFlags;

		VK_CHECK(vmaCreateBuffer(manager.get_allocator(), &bufferInfo, &vmaAllocInfo,
			&buffer->buffer, &buffer->allocation, nullptr));
	}

	void map_memory(VulkanManager &manager, AllocatedBuffer &buffer, std::function<void(void *data)> func)
	{
		void *data;

		vmaMapMemory(manager.get_allocator(), buffer.allocation, &data);
		func(data);
		vmaUnmapMemory(manager.get_allocator(), buffer.allocation);
	}

	void map_memory(VulkanManager &manager, AllocatedBuffer &buffer, void *memory, uint32_t size)
	{
		map_memory(manager, buffer, [=](void *data) {
			memcpy(data, memory, size);
		});
	}

	void destroy_buffer(VulkanManager &manager, AllocatedBuffer &buffer)
	{
		vmaDestroyBuffer(manager.get_allocator(), buffer.buffer, buffer.allocation);
	}

	void upload_to_gpu(VulkanManager &manager, void *copyData, uint32_t size, AllocatedBuffer &buffer, VkBufferUsageFlags flags)
	{
		VkBufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = size;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBuffer stagingBuffer;

		create_buffer(manager, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);

		map_memory(manager, stagingBuffer, copyData, size);

		create_buffer(manager, size, flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffer);

		manager.immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy{};
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = size;
		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, buffer.buffer, 1, &copy);
		});

		vmaDestroyBuffer(manager.get_allocator(), stagingBuffer.buffer, stagingBuffer.allocation);
	}

	void staged_upload_to_buffer(VulkanManager &manager, AllocatedBuffer &buffer, void *copyData, uint32_t size)
	{
		VkBufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = size;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		AllocatedBuffer stagingBuffer;
		create_buffer(manager, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);

		map_memory(manager, stagingBuffer, copyData, size);

		manager.immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy{};
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = size;
		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, buffer.buffer, 1, &copy);
		});

		vmaDestroyBuffer(manager.get_allocator(), stagingBuffer.buffer, stagingBuffer.allocation);
	}

	void create_image(VulkanManager &manager, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags, AllocatedImage *img)
	{

		VkExtent3D imageExtent{};
		imageExtent.width = width;
		imageExtent.height = height;
		imageExtent.depth = 1;

		VkImageCreateInfo dimgInfo = vkinit::image_create_info(format, flags, imageExtent);

		VmaAllocationCreateInfo dimgAllocInfo{};
		dimgAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		vmaCreateImage(manager.get_allocator(), &dimgInfo, &dimgAllocInfo, &img->image, &img->allocation, nullptr);
	}

	TextureCreateInfo color_texture_create_info(uint32_t w, uint32_t h, VkFormat format)
	{
		TextureCreateInfo info{};
		info.width = w;
		info.height = h;
		info.format = format;
		info.createImguiDescriptor = true;
		info.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		info.filter = VK_FILTER_LINEAR;
		info.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		return info;
	}

	TextureCreateInfo depth_texture_create_info(uint32_t w, uint32_t h, VkFormat format)
	{
		TextureCreateInfo info{};
		info.width = w;
		info.height = h;
		info.format = format;
		info.filter = VK_FILTER_LINEAR;
		info.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		info.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.createImguiDescriptor = false;

		return info;
	}


	void set_texture_data(VulkanManager &manager, VkTexture &tex, void *data) {
		VkDeviceSize imageSize = (uint64_t)(tex.width * tex.height * 4);

		VkExtent3D imageExtent{};
		imageExtent.width = tex.width;
		imageExtent.height = tex.height;
		imageExtent.depth = 1;

		AllocatedBuffer stagingBuffer;
		create_buffer(manager, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);

		map_memory(manager, stagingBuffer, data, (uint32_t)imageSize);

		manager.immediate_submit([&](VkCommandBuffer cmd) {
			VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier imageBarrierToTransfer{};
		imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrierToTransfer.image = tex.imageAllocation.image;
		imageBarrierToTransfer.subresourceRange = range;

		imageBarrierToTransfer.srcAccessMask = 0;
		imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &imageBarrierToTransfer);

		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = imageExtent;

		vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, tex.imageAllocation.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;

		imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
			0, nullptr, 1, &imageBarrierToReadable);
		});

		vmaDestroyBuffer(manager.get_allocator(), stagingBuffer.buffer, stagingBuffer.allocation);
	}

	void destroy_texture(const VulkanManager &manager, VkTexture &tex) {
		vkDestroyImageView(manager.device(), tex.imageView, nullptr);
		vmaDestroyImage(manager.get_allocator(), tex.imageAllocation.image, tex.imageAllocation.allocation);

		if (tex.bImguiDescriptor) {
			ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)tex.imguiDescriptor);
			vkDestroySampler(manager.device(), tex.sampler, nullptr);
		}
	}

	void insert_image_memory_barrier(VkCommandBuffer command_buffer, VkImage image, VkAccessFlags src_access_mask,
		VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout,
		VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask, VkImageSubresourceRange range)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = src_access_mask;
		barrier.dstAccessMask = dst_access_mask;
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;
		barrier.image = image;
		barrier.subresourceRange = range;
		//barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//barrier.subresourceRange.levelCount = 1;
		//barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			command_buffer,
			src_stage_mask,
			dst_stage_mask,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}


	void alloc_texture(VulkanManager &manager, TextureCreateInfo &info, VkTexture *tex)
	{
		tex->width = info.width;
		tex->height = info.height;
		tex->format = info.format;
		tex->bImguiDescriptor = info.createImguiDescriptor;

		if (info.createImguiDescriptor)
			info.usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		create_image(manager, info.width, info.height, info.format, info.usageFlags, &tex->imageAllocation);

		VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(
			info.format, tex->imageAllocation.image,
			info.aspectFlags);

		vkCreateImageView(manager.device(), &imageInfo, nullptr, &tex->imageView);


		if (info.createImguiDescriptor) {
			VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(info.filter);
			VK_CHECK(vkCreateSampler(manager.device(), &samplerInfo, nullptr, &tex->sampler));
			tex->imguiDescriptor = ImGui_ImplVulkan_AddTexture(tex->sampler, tex->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		if (info.usageFlags & (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) {

			manager.immediate_submit([=](VkCommandBuffer cmd) {
				VkImageSubresourceRange range{};
			range.aspectMask = info.aspectFlags;
			range.levelCount = 1;
			range.layerCount = 1;

			insert_image_memory_barrier(cmd, tex->imageAllocation.image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				range);
			});
		}
	}

	bool load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info, VkTexture *tex) {
		//Ref<Texture> tex = make_ref<Texture>();
		CORE_ASSERT(tex->imageAllocation.image == VK_NULL_HANDLE, "VkImage is not VK_NULL_HANDLE, overriding not allowed");

		tex->format = VK_FORMAT_R8G8B8A8_UNORM;

		int w, h, nC;
		if (!load_alloc_image_from_file(file, manager, &tex->imageAllocation, &w, &h, &nC, tex->format)) {
			return false;
		}

		tex->width = static_cast<uint32_t>(w);
		tex->height = static_cast<uint32_t>(h);

		VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(
			tex->format, tex->imageAllocation.image,
			VK_IMAGE_ASPECT_COLOR_BIT);

		vkCreateImageView(manager.device(), &imageInfo, nullptr, &tex->imageView);

		VK_CHECK(vkCreateSampler(manager.device(), &info, nullptr, &tex->sampler));

		tex->imguiDescriptor = ImGui_ImplVulkan_AddTexture(tex->sampler, tex->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		tex->bImguiDescriptor = true;

		return true;
	}

	bool load_alloc_image_from_file(const char *file, VulkanManager &manager,
		AllocatedImage *outImage, int *w, int *h, int *nC, VkFormat format) {

		stbi_uc *pixel_ptr = stbi_load(file, w, h, nC, STBI_rgb_alpha);

		int width = *w;
		int height = *h;
		int nChannels = *nC;

		if (!pixel_ptr) {
			CORE_WARN("Failed to load texture file: {}, message: {}", file, stbi_failure_reason());
			return false;
		}

		VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

		AllocatedBuffer stagingBuffer;
		create_buffer(manager, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);

		map_memory(manager, stagingBuffer, pixel_ptr, (uint32_t)imageSize);

		stbi_image_free(pixel_ptr);

		VkExtent3D imageExtent{};
		imageExtent.width = static_cast<uint32_t>(width);
		imageExtent.height = static_cast<uint32_t>(height);
		imageExtent.depth = 1;

		AllocatedImage img;
		create_image(manager, imageExtent.width, imageExtent.height, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, &img);

		manager.immediate_submit([&](VkCommandBuffer cmd) {
			VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier imageBarrierToTransfer{};
		imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrierToTransfer.image = img.image;
		imageBarrierToTransfer.subresourceRange = range;

		imageBarrierToTransfer.srcAccessMask = 0;
		imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &imageBarrierToTransfer);

		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = imageExtent;

		vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, img.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;

		imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
			0, nullptr, 1, &imageBarrierToReadable);
		});

		vmaDestroyBuffer(manager.get_allocator(), stagingBuffer.buffer, stagingBuffer.allocation);

		*outImage = img;

		return true;
	}


}

namespace vkinit {

	VkCommandPoolCreateInfo
		command_pool_create_info(uint32_t queueFamilyIndex,
			VkCommandPoolCreateFlags flags) {

		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;

		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = flags;
		return info;
	}

	VkCommandBufferAllocateInfo
		command_buffer_allocate_info(VkCommandPool pool, uint32_t count,
			VkCommandBufferLevel level) {

		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext = nullptr;

		info.commandPool = pool;
		info.commandBufferCount = count;
		info.level = level;
		return info;
	}

	VkCommandBufferBeginInfo
		command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
		VkCommandBufferBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext = nullptr;

		info.pInheritanceInfo = nullptr;
		info.flags = flags;
		return info;
	}

	VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass renderPass,
		VkExtent2D extent) {
		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.pNext = nullptr;

		info.renderPass = renderPass;
		info.attachmentCount = 1;
		info.width = extent.width;
		info.height = extent.height;
		info.layers = 1;

		return info;
	}

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags) {
		VkFenceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = flags;
		return info;
	}

	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags) {
		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = flags;
		return info;
	}

	VkSubmitInfo submit_info(VkCommandBuffer *cmd) {
		VkSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;

		info.waitSemaphoreCount = 0;
		info.pWaitSemaphores = nullptr;
		info.pWaitDstStageMask = nullptr;
		info.commandBufferCount = 1;
		info.pCommandBuffers = cmd;
		info.signalSemaphoreCount = 0;
		info.pSignalSemaphores = nullptr;

		return info;
	}

	VkPresentInfoKHR present_info() {
		VkPresentInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.pNext = nullptr;

		info.swapchainCount = 0;
		info.pSwapchains = nullptr;
		info.pWaitSemaphores = nullptr;
		info.waitSemaphoreCount = 0;
		info.pImageIndices = nullptr;

		return info;
	}

	VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass,
		VkExtent2D windowExtent,
		VkFramebuffer framebuffer) {
		VkRenderPassBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.pNext = nullptr;

		info.renderPass = renderPass;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		info.renderArea.extent = windowExtent;
		info.clearValueCount = 1;
		info.pClearValues = nullptr;
		info.framebuffer = framebuffer;

		return info;
	}

	VkPipelineShaderStageCreateInfo
		pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {

		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.stage = stage;
		info.module = shaderModule;
		info.pName = "main"; // entry point of the shader

		return info;
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info() {
		VkPipelineVertexInputStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.vertexBindingDescriptionCount = 0;
		info.vertexAttributeDescriptionCount = 0;
		return info;
	}

	VkPipelineInputAssemblyStateCreateInfo
		input_assembly_create_info(VkPrimitiveTopology topology) {
		VkPipelineInputAssemblyStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.topology = topology;
		info.primitiveRestartEnable = VK_FALSE; // never used
		return info;
	}

	VkPipelineRasterizationStateCreateInfo
		rasterization_state_create_info(VkPolygonMode polygonMode) {
		// TODO: check cull + depth settings
		VkPipelineRasterizationStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.depthClampEnable = VK_FALSE;
		// discards all primitives before the rasterization stage if enabled which we
		// don't want
		info.rasterizerDiscardEnable = VK_FALSE;

		info.polygonMode = polygonMode;
		info.lineWidth = 1.0f;

		info.cullMode = VK_CULL_MODE_NONE;
		info.frontFace = VK_FRONT_FACE_CLOCKWISE;

		info.depthBiasEnable = VK_FALSE;
		info.depthBiasConstantFactor = 0.0f;
		info.depthBiasClamp = 0.0f;
		info.depthBiasSlopeFactor = 0.0f;

		return info;
	}

	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info() {
		// TODO: to enable this set rasterizationSamples to more than 1
		//  has to be supported by renderpass
		VkPipelineMultisampleStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.sampleShadingEnable = VK_FALSE;
		info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		info.minSampleShading = 1.0f;
		info.pSampleMask = nullptr;
		info.alphaToCoverageEnable = VK_FALSE;
		info.alphaToOneEnable = VK_FALSE;
		return info;
	}

	VkPipelineColorBlendAttachmentState color_blend_attachment_state() {
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		return colorBlendAttachment;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info() {
		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;

		info.flags = 0;
		info.setLayoutCount = 0;
		info.pSetLayouts = nullptr;
		info.pushConstantRangeCount = 0;
		info.pPushConstantRanges = nullptr;
		return info;
	}

	VkImageCreateInfo image_create_info(VkFormat format,
		VkImageUsageFlags usageFlags,
		VkExtent3D extent) {
		VkImageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.format = format;
		info.extent = extent;

		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL; // use linear for access from the cpu
		info.usage = usageFlags;

		return info;
	}
	VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image,
		VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;

		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.image = image;
		info.format = format;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		info.subresourceRange.aspectMask = aspectFlags;

		return info;
	}
	VkPipelineDepthStencilStateCreateInfo
		depth_stencil_create_info(bool depthTest, bool depthWrite,
			VkCompareOp compareOp) {
		VkPipelineDepthStencilStateCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
		info.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
		info.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
		info.depthBoundsTestEnable = VK_FALSE;
		info.minDepthBounds = 0.0f; // Optional
		info.maxDepthBounds = 1.0f; // Optional
		info.stencilTestEnable = VK_FALSE;

		return info;
	}
	VkDescriptorSetLayoutBinding
		descriptorset_layout_binding(VkDescriptorType type,
			VkShaderStageFlags stageFlags, uint32_t binding) {
		VkDescriptorSetLayoutBinding setbind{};
		setbind.binding = binding;
		setbind.descriptorCount = 1;
		setbind.descriptorType = type;
		setbind.pImmutableSamplers = nullptr;
		setbind.stageFlags = stageFlags;

		return setbind;
	}
	VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type,
		VkDescriptorSet dstSet,
		VkDescriptorBufferInfo *bufferInfo,
		uint32_t binding) {
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;

		write.dstBinding = binding;
		write.dstSet = dstSet;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = bufferInfo;

		return write;
	}

	VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAdressMode /*= VK_SAMPLER_ADDRESS_MODE_REPEAT*/) {
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.pNext = nullptr;

		info.magFilter = filters;
		info.minFilter = filters;
		info.addressModeU = samplerAdressMode;
		info.addressModeV = samplerAdressMode;
		info.addressModeW = samplerAdressMode;

		return info;
	}

	VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type,
		VkDescriptorSet dstSet,
		VkDescriptorImageInfo *imageInfo,
		uint32_t binding) {
		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;

		write.dstBinding = binding;
		write.dstSet = dstSet;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = imageInfo;

		return write;
	}
} // namespace vkinit
