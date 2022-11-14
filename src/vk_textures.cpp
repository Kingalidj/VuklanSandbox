#include "vk_textures.h"

#include "vk_manager.h"
#include "vk_initializers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vkutil {

	TextureCreateInfo color_texture_create_info(uint32_t w, uint32_t h, VkFormat format)
	{
		TextureCreateInfo info;
		info.width = w;
		info.height = h;
		info.format = format;
		info.createImguiDescriptor = true;
		info.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		info.filter = VK_FILTER_LINEAR;
		info.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		return info;
	}

	TextureCreateInfo depth_texture_create_info(uint32_t w, uint32_t h, VkFormat format)
	{
		vkutil::TextureCreateInfo info{};
		info.width = w;
		info.height = h;
		info.format = format;
		info.filter = VK_FILTER_LINEAR;
		info.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		info.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		info.createImguiDescriptor = false;

		return info;
	}


	void set_texture_data(Texture &tex, const void *data, VulkanManager &manager) {
		VkDeviceSize imageSize = tex.width * tex.height * 4;

		VkExtent3D imageExtent;
		imageExtent.width = tex.width;
		imageExtent.height = tex.height;
		imageExtent.depth = 1;

		AllocatedBuffer stagingBuffer;
		manager.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer);

		void *map;
		vmaMapMemory(manager.get_allocator(), stagingBuffer.allocation, &map);
		memcpy(map, data, static_cast<size_t>(imageSize));
		vmaUnmapMemory(manager.get_allocator(), stagingBuffer.allocation);

		manager.immediate_submit([&](VkCommandBuffer cmd) {
			VkImageSubresourceRange range;
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

	void destroy_texture(VulkanManager &manager, Texture &tex) {
		vkDestroyImageView(manager.get_device(), tex.imageView, nullptr);
		vmaDestroyImage(manager.get_allocator(), tex.imageAllocation.image, tex.imageAllocation.allocation);

		if (tex.imguiCompatible) {
			ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)tex.imGuiTexID);
			vkDestroySampler(manager.get_device(), tex.imGuiSampler, nullptr);
		}
	}

	void alloc_texture(VulkanManager& manager, TextureCreateInfo &info, Texture* tex)
	{
		tex->width = info.width;
		tex->height = info.height;
		tex->format = info.format;

		manager.create_image(info.width, info.height, info.format, info.usageFlags, &tex->imageAllocation);

		VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(
			info.format, tex->imageAllocation.image,
			info.aspectFlags);

		vkCreateImageView(manager.get_device(), &imageInfo, nullptr, &tex->imageView);

		tex->imguiCompatible = false;

		if (info.createImguiDescriptor) {
			tex->imguiCompatible = true;

			VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(info.filter);
			VK_CHECK(vkCreateSampler(manager.get_device(), &samplerInfo, nullptr, &tex->imGuiSampler));
			tex->imGuiTexID = ImGui_ImplVulkan_AddTexture(tex->imGuiSampler, tex->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	std::optional<Ref<Texture>> load_texture(const char *file, VulkanManager &manager, VkSamplerCreateInfo &info) {
		Ref<Texture> tex = make_ref<Texture>();

		//VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		tex->format = VK_FORMAT_R8G8B8A8_UNORM;

		int w, h, nC;
		if (!load_alloc_image_from_file(file, manager, &tex->imageAllocation, &w, &h, &nC)) {
			CORE_WARN("Could not load Texture: {}", file);
			return std::nullopt;
		}

		tex->width = static_cast<uint32_t>(w);
		tex->height = static_cast<uint32_t>(h);

		VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(
			tex->format, tex->imageAllocation.image,
			VK_IMAGE_ASPECT_COLOR_BIT);

		vkCreateImageView(manager.get_device(), &imageInfo, nullptr, &tex->imageView);

		VK_CHECK(vkCreateSampler(manager.get_device(), &info, nullptr, &tex->imGuiSampler));

		tex->imGuiTexID = ImGui_ImplVulkan_AddTexture(tex->imGuiSampler, tex->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		tex->imguiCompatible = true;

		return std::move(tex);
	}

	bool load_alloc_image_from_file(const char *file, VulkanManager &manager,
		AllocatedImage *outImage, int *w, int *h, int *nC) {

		stbi_uc *pixel_ptr =
			stbi_load(file, w, h, nC, STBI_rgb_alpha);

		int width = *w;
		int height = *h;
		int nChannels = *nC;

		if (!pixel_ptr) {
			CORE_WARN("Failed to load texture file: {}", file);
			return false;
		}

		VkDeviceSize imageSize = width * height * 4;

		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

		AllocatedBuffer stagingBuffer;
		manager.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer);

		void *data;
		vmaMapMemory(manager.get_allocator(), stagingBuffer.allocation, &data);
		memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
		vmaUnmapMemory(manager.get_allocator(), stagingBuffer.allocation);

		stbi_image_free(pixel_ptr);

		VkExtent3D imageExtent;
		imageExtent.width = static_cast<uint32_t>(width);
		imageExtent.height = static_cast<uint32_t>(height);
		imageExtent.depth = 1;

		AllocatedImage img;
		manager.create_image(imageExtent.width, imageExtent.height, imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, &img);

		manager.immediate_submit([&](VkCommandBuffer cmd) {
			VkImageSubresourceRange range;
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
} // namespace vkutil
