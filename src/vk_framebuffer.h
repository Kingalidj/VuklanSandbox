#pragma once

#include "vk_types.h"
#include "vk_textures.h"
#include "vk_manager.h"

namespace vkutil {

	struct Framebuffer {
		std::vector<Texture> framebufferTexture;
		VkFramebuffer framebuffer{ VK_NULL_HANDLE };
	};

	class FramebufferBuilder {
	public:

		FramebufferBuilder(uint32_t w, uint32_t h, VkRenderPass rp)
			:width(w), height(h), renderpass(rp) {}

		FramebufferBuilder &push_attachment(TextureCreateInfo &info);
		Framebuffer build(VulkanManager &manager);

	private:

		uint32_t width, height;
		VkRenderPass renderpass;

		std::vector<TextureCreateInfo> m_AttachmentInfos;
	};

	void destroy_framebuffer(VulkanManager &manager, Framebuffer &framebuffer);

}
