#include "vk_framebuffer.h"

#include "vk_initializers.h"

namespace vkutil {

	FramebufferBuilder &FramebufferBuilder::push_attachment(TextureCreateInfo &info)
	{
		m_AttachmentInfos.push_back(info);
		return *this;
	}

	Framebuffer FramebufferBuilder::build(VulkanManager &manager)
	{
		Framebuffer framebuffer;

		framebuffer.framebufferTexture = std::vector<Texture>(m_AttachmentInfos.size());
		std::vector<VkImageView> attachmentViews = std::vector<VkImageView>(m_AttachmentInfos.size());

		for (uint32_t i = 0; i < m_AttachmentInfos.size(); i++) {
			alloc_texture(manager, m_AttachmentInfos[i], &framebuffer.framebufferTexture[i]);
			attachmentViews[i] = framebuffer.framebufferTexture[i].imageView;
		}

		VkFramebufferCreateInfo fbInfo = vkinit::framebuffer_create_info(renderpass, { width, height });
		fbInfo.pAttachments = attachmentViews.data();
		fbInfo.attachmentCount = attachmentViews.size();

		VK_CHECK(vkCreateFramebuffer(manager.get_device(), &fbInfo, nullptr, &framebuffer.framebuffer));

		return framebuffer;
	}


	void destroy_framebuffer(VulkanManager &manager, Framebuffer &framebuffer)
	{
		vkDestroyFramebuffer(manager.get_device(), framebuffer.framebuffer, nullptr);
		for (auto &tex : framebuffer.framebufferTexture) {
			destroy_texture(manager, tex);
		}
	}

}
