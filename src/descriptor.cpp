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

		VulkanDescriptor(Atlas::DescriptorBindings &bindings) {

			auto builder = DescriptorBuilder(Atlas::Application::get_engine().manager());

			uint32_t binding = 0;

			for (auto &pair : bindings) {

				auto stage = atlas_to_vk_shaderstage(pair.second);

				if (std::holds_alternative<Ref<Atlas::Buffer>>(pair.first)) { //buffer variant

					Ref<Atlas::Buffer> buffer = std::get<Ref<Atlas::Buffer>>(pair.first);
					if (!buffer->is_init()) {
						CORE_WARN("Descriptor: buffer is not initialized!");
						return;
					}

					m_Buffers.insert({ binding, buffer });

					builder.bind_buffer(binding++, *buffer->get_native_buffer(), buffer->size(),
						atlas_to_vk_descriptor_type(buffer->get_type()), stage);
				}
				else if (std::holds_alternative<Ref<Atlas::Texture>>(pair.first)) { //texture variant

					Ref<Atlas::Texture> texture = std::get<Ref<Atlas::Texture>>(pair.first);
					if (!texture->is_init()) {
						CORE_WARN("Descriptor: texture is not initialized!");
						return;
					}

					m_Textures.insert({ binding, texture });

					builder.bind_image(binding++, *texture->get_native_texture(),
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
				}
				else if (std::holds_alternative<std::vector<Ref<Atlas::Texture>>>(pair.first)) {

					std::vector<Ref<Atlas::Texture>> textures =
						std::get<std::vector<Ref<Atlas::Texture>>>(pair.first);
					m_TextureArrays.insert({ binding, textures });

					std::vector<Texture> vulkanTextures;

					for (auto &tex : textures) {
						if (!tex->is_init()) {
							CORE_WARN("Descriptor: texture is not initialized!");
							return;
						}
						vulkanTextures.push_back(*tex->get_native_texture());
					}

					builder.bind_image_array(binding++, vulkanTextures.data(), (uint32_t)vulkanTextures.size(),
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
				}
			}

			builder.build(&m_Descriptor.set, &m_Descriptor.layout);
		}

		void update(uint32_t binding, Atlas::DescriptorBinding descBinding) {

			auto stage = atlas_to_vk_shaderstage(descBinding.second);

			if (std::holds_alternative<Ref<Atlas::Buffer>>(descBinding.first)) {
				Ref<Atlas::Buffer> buffer = std::get<Ref<Atlas::Buffer>>(descBinding.first);

				m_Buffers.at(binding) = buffer;

				descriptor_update_buffer(Atlas::Application::get_engine().manager(), &m_Descriptor.set, binding,
					*buffer->get_native_buffer(), buffer->size(), atlas_to_vk_descriptor_type(buffer->get_type()), stage);
			}
			else if (std::holds_alternative<Ref<Atlas::Texture>>(descBinding.first)) {
				Ref<Atlas::Texture> texture = std::get<Ref<Atlas::Texture>>(descBinding.first);

				m_Textures.at(binding) = texture;

				descriptor_update_image(Atlas::Application::get_engine().manager(), &m_Descriptor.set, binding,
					*texture->get_native_texture(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
			}
			else if (std::holds_alternative<std::vector<Ref<Atlas::Texture>>>(descBinding.first)) {
				std::vector<Ref<Atlas::Texture>> textures =
					std::get<std::vector<Ref<Atlas::Texture>>>(descBinding.first);

				m_TextureArrays.at(binding) = textures;

				std::vector<Texture> vulkanTextures;
				for (auto &tex : textures) vulkanTextures.push_back(*tex->get_native_texture());

				descriptor_update_image_array(Atlas::Application::get_engine().manager(), &m_Descriptor.set, binding,
					vulkanTextures.data(), (uint32_t)vulkanTextures.size(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
			}
		}

		Descriptor *get_native() {
			return &m_Descriptor;
		}

		uint32_t get_descriptor_count() {
			return (uint32_t)(m_Buffers.size() + m_Textures.size());
		}

	private:
		std::unordered_map<uint32_t, Ref<Atlas::Buffer>> m_Buffers;
		std::unordered_map<uint32_t, Ref<Atlas::Texture>> m_Textures;
		std::unordered_map<uint32_t, std::vector<Ref<Atlas::Texture>>> m_TextureArrays;

		vkutil::Descriptor m_Descriptor;
	};

}

namespace Atlas {

	Descriptor::Descriptor(DescriptorBindings &bindings)
		: m_Initialized(true)
	{
		m_Descriptor = make_ref<vkutil::VulkanDescriptor>(bindings);
	}

	uint32_t Descriptor::get_descriptor_count()
	{
		if (!m_Initialized) {
			CORE_WARN("Descriptor was not initialized");
			return 0;
		}

		return m_Descriptor->get_descriptor_count();
	}

	void Descriptor::update(uint32_t binding, DescriptorBinding descBinding)
	{
		m_Descriptor->update(binding, descBinding);
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
