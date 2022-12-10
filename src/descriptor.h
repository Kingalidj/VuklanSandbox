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
	using DescriptorBinding = std::pair<DescriptorAttachment, ShaderStage>;
	using DescriptorBindings = std::vector<DescriptorBinding>;

	//struct DescriptorCreateInfo {
	//	std::vector<std::pair<DescriptorAttachment, ShaderStage>> bindings;
	//};

	class Descriptor {
	public:

		Descriptor() = default;

		Descriptor(DescriptorBindings &bindings);
		uint32_t get_descriptor_count();
		void update(uint32_t binding, DescriptorBinding descBinding);

		inline bool is_init() { return m_Initialized; }

		vkutil::Descriptor *get_native_descriptor();

	private:

		bool m_Initialized{ false };
		Ref<vkutil::VulkanDescriptor> m_Descriptor;

	};

}
