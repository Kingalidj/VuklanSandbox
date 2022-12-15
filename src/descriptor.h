#pragma once

#include "buffer.h"
#include "shader.h"
#include "texture.h"

namespace vkutil {
	class VulkanDescriptor;
	struct VkDescriptor;
}

namespace Atlas {

	class Descriptor {
	public:

		using BufferAttachment = Ref<Buffer>;
		using TextureAttachment = Ref<Texture>;
		using TextureArrayAttachment = std::vector<Ref<Texture>>;
		using Attachment = std::variant<BufferAttachment, TextureAttachment, TextureArrayAttachment>;
		using Binding = std::pair<Attachment, ShaderStage>;
		using Bindings = std::vector<Binding>;

		Descriptor() = default;

		Descriptor(std::vector<Binding> &bindings, bool pushable = false);
		uint32_t get_set_count();

		void update(uint32_t binding, Binding descBinding);
		void bind(Shader &shader);
		void push(Shader &shader, uint32_t set);

		inline bool is_init() { return m_Initialized; }

		vkutil::VkDescriptor *get_native_descriptor();

	private:

		bool m_Initialized{ false };
		Ref<vkutil::VulkanDescriptor> m_Descriptor;

	};

}
