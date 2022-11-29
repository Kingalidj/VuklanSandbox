#include "descriptor.h"
#include "buffer.h"
#include "shader.h"
#include "application.h"

#include "vk_engine.h"
#include "atl_vk_utils.h"


namespace vkutil {

	class VulkanDescriptor {
	public:

		using AttachmentType = Ref<Atlas::Buffer>;

		VulkanDescriptor(Atlas::DescriptorCreateInfo &info) {

			auto builder = DescriptorBuilder(Atlas::Application::get_engine().manager());

			uint32_t binding = 0;

			for (auto &pair : info.bindings) {
				if (pair.first.index() == 0) {

					Ref<Atlas::Buffer> buffer = std::get<Ref<Atlas::Buffer>>(pair.first);
					if (!buffer->is_init()) {
						CORE_WARN("Descriptor: buffer is not initialized!");
						return;
					}

					m_Buffers.push_back(buffer);

					builder.bind_buffer(binding++, *buffer->get_native_buffer(), buffer->size(),
						atlas_to_vk_descriptor_type(buffer->get_type()), atlas_to_vk_shaderstage(pair.second));
				}
				else if (pair.first.index() == 1) {

					Ref<Atlas::Texture> texture = std::get<Ref<Atlas::Texture>>(pair.first);
					if (!texture->is_init()) {
						CORE_WARN("Descriptor: texture is not initialized!");
						return;
					}

					m_Textures.push_back(texture);

					builder.bind_image(binding++, *texture->get_native_texture(),
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, atlas_to_vk_shaderstage(pair.second));
				}
			}

			builder.build(&m_Descriptor.set, &m_Descriptor.layout);
		}

		Descriptor *get_native() {
			return &m_Descriptor;
		}

		uint32_t get_descriptor_count() {
			return m_Buffers.size() + m_Textures.size();
		}

	private:
		std::vector<Ref<Atlas::Buffer>> m_Buffers;
		std::vector<Ref<Atlas::Texture>> m_Textures;

		vkutil::Descriptor m_Descriptor;

	};

}

namespace Atlas {

	Descriptor::Descriptor(std::vector<std::pair<DescriptorAttachment, ShaderStage>> bindings)
		: m_Initialized(true)
	{
		DescriptorCreateInfo info{};
		info.bindings = bindings;

		m_Descriptor = make_ref<vkutil::VulkanDescriptor>(info);
	}

	Descriptor::Descriptor(DescriptorCreateInfo &info)
		: m_Initialized(true)
	{
		m_Descriptor = make_ref<vkutil::VulkanDescriptor>(info);
	}

	uint32_t Descriptor::get_descriptor_count()
	{
		if (!m_Initialized) {
			CORE_WARN("Descriptor was not initialized");
			return 0;
		}

		return m_Descriptor->get_descriptor_count();
	}

	vkutil::Descriptor *Descriptor::get_native_descriptor()
	{
		if (!m_Initialized) {
			CORE_WARN("Descriptor was not initialized");
			return nullptr;
		}

		return m_Descriptor->get_native();
	}

}
