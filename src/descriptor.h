#pragma once

#include "buffer.h"
#include "shader.h"
#include "texture.h"

namespace vkutil {
	class VulkanDescriptor;
	struct Descriptor;
}

namespace Atlas {

	using DescriptorAttachment = std::variant<Ref<Buffer>, Ref<Texture>, std::vector<Ref<Texture>>>;

	enum class DescriptorAttachmentType : uint32_t {
		BUFFER = 0,
		TEXTURE,
		TEXTURE_ARRAY,
	};

	struct DescriptorCreateInfo {
		std::vector<std::pair<DescriptorAttachment, ShaderStage>> bindings;

	};

	class Descriptor {
	public:

		Descriptor(std::vector<std::pair<DescriptorAttachment, ShaderStage>> bindings);
		Descriptor(DescriptorCreateInfo &info);

		Descriptor(Descriptor &other) = delete;

		uint32_t get_descriptor_count();

		//TODO: return void*
		vkutil::Descriptor *get_native_descriptor();

	private:

		bool m_Initialized{ false };
		Ref<vkutil::VulkanDescriptor> m_Descriptor;

	};

}
