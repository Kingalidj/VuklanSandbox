#include "descriptor.h"
#include "buffer.h"
#include "shader.h"
#include "application.h"

#include "vk_engine.h"
#include "vk_descriptors.h"
#include "atl_vk_utils.h"


namespace vkutil {
	using namespace Atlas;

	class VulkanDescriptor {
	public:

		VulkanDescriptor(std::vector<Descriptor::Binding> &bindings, bool pushable)
			:m_Pushable(pushable)
		{

			auto builder = DescriptorBuilder(Application::get_engine().manager());

			if (pushable)
				builder.enable_push_descriptor();

			uint32_t binding = 0;

			for (auto &pair : bindings) {
				auto stage = atlas_to_vk_shaderstage(pair.second);

				if (std::holds_alternative<Descriptor::BufferAttachment>(pair.first)) { //buffer variant
					Descriptor::BufferAttachment buffer = std::get<Descriptor::BufferAttachment>(pair.first);
					if (!buffer->is_init()) {
						CORE_WARN("Descriptor: buffer is not initialized!");
						return;
					}
					m_Attachments.push_back(buffer);
					builder.bind_buffer(binding++, *buffer->get_native_buffer(), buffer->size(),
						atlas_to_vk_descriptor_type(buffer->get_type()), stage);
				}

				else if (std::holds_alternative<Descriptor::TextureAttachment>(pair.first)) { //texture variant
					Descriptor::TextureAttachment texture = std::get<Descriptor::TextureAttachment>(pair.first);
					if (!texture->is_init()) {
						CORE_WARN("Descriptor: texture is not initialized!");
						return;
					}
					m_Attachments.push_back(texture);
					builder.bind_image(binding++, *texture->get_native_texture(),
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
				}

				else if (std::holds_alternative<Descriptor::TextureArrayAttachment>(pair.first)) {
					Descriptor::TextureArrayAttachment textures = std::get<Descriptor::TextureArrayAttachment>(pair.first);
					std::vector<VkTexture> vulkanTextures;
					for (auto &tex : textures) {
						if (!tex->is_init()) {
							CORE_WARN("Descriptor: texture is not initialized!");
							return;
						}
						vulkanTextures.push_back(*tex->get_native_texture());
					}
					m_Attachments.push_back(textures);
					builder.bind_image_array(binding++, vulkanTextures.data(), (uint32_t)vulkanTextures.size(),
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
				}
			}

			builder.build(&m_Descriptor.set, &m_Descriptor.layout);
		}

		void update(uint32_t binding, Descriptor::Binding descBinding) {

			CORE_ASSERT(binding < (uint32_t)m_Attachments.size(), "Descriptor: binding is out of bounds");

			auto stage = atlas_to_vk_shaderstage(descBinding.second);

			if (std::holds_alternative<Descriptor::BufferAttachment>(descBinding.first)) {
				Descriptor::BufferAttachment buffer = std::get<Descriptor::BufferAttachment>(descBinding.first);
				if (m_Attachments.at(binding).index() != descBinding.first.index()) {
					CORE_WARN("Descriptor: binding {} can not be updated because the layout is incompatible with Buffer!", binding);
					return;
				}
				m_Attachments.at(binding) = buffer;

				if (!m_Pushable) {
					descriptor_update_buffer(Application::get_engine().manager(), &m_Descriptor.set, binding,
						*buffer->get_native_buffer(), buffer->size(), atlas_to_vk_descriptor_type(buffer->get_type()), stage);
				}
			}

			else if (std::holds_alternative<Descriptor::TextureAttachment>(descBinding.first)) {
				Descriptor::TextureAttachment texture = std::get<Descriptor::TextureAttachment>(descBinding.first);
				if (m_Attachments.at(binding).index() != descBinding.first.index()) {
					CORE_WARN("Descriptor: binding {} can not be updated because the layout is incompatible with Texture!", binding);
					return;
				}
				m_Attachments.at(binding) = texture;

				if (!m_Pushable) {
					descriptor_update_image(Application::get_engine().manager(), &m_Descriptor.set, binding,
						*texture->get_native_texture(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
				}
			}

			else if (std::holds_alternative<Descriptor::TextureArrayAttachment>(descBinding.first)) {
				Descriptor::TextureArrayAttachment textures = std::get<Descriptor::TextureArrayAttachment>(descBinding.first);
				if (m_Attachments.at(binding).index() != descBinding.first.index()) {
					CORE_WARN("Descriptor: binding {} can not be updated because the layout is incompatible with TextureArray!", binding);
					return;
				}
				m_Attachments.at(binding) = textures;
				std::vector<VkTexture> vulkanTextures;
				for (auto &tex : textures) vulkanTextures.push_back(*tex->get_native_texture());

				if (!m_Pushable) {
					descriptor_update_image_array(Application::get_engine().manager(), &m_Descriptor.set, binding,
						vulkanTextures.data(), (uint32_t)vulkanTextures.size(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage);
				}
			}
		}

		void bind(Atlas::Shader &shader) {
			CORE_ASSERT(!m_Pushable, "Descriptor: pushDescriptorFlag can not to be set");

			VkCommandBuffer cmd = Application::get_engine().get_active_command_buffer();
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				shader.get_native_shader()->layout, 0, 1,
				&m_Descriptor.set, 0, nullptr);
		}

		void push(Atlas::Shader &shader, uint32_t set) {

			CORE_ASSERT(m_Pushable, "Descriptor: pushDescriptorFlag has to be set");

			uint32_t size = (uint32_t)m_Attachments.size();

			std::vector<VkWriteDescriptorSet> writes;

			std::unordered_map<uint32_t, VkDescriptorBufferInfo> bufferInfos;
			std::unordered_map<uint32_t, VkDescriptorImageInfo> imageInfos;
			std::unordered_map<uint32_t, std::vector<VkDescriptorImageInfo>> imageArrayInfos;

			for (uint32_t i = 0; i < size; i++) {
				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = 0;
				write.dstBinding = i;

				auto &at = m_Attachments.at(i);
				if (std::holds_alternative<Descriptor::BufferAttachment>(at)) { //buffer variant
					Descriptor::BufferAttachment buffer = std::get<Descriptor::BufferAttachment>(at);
					auto info = descriptor_buffer_info(*buffer->get_native_buffer(), (uint32_t)buffer->size());
					auto [it, ex] = bufferInfos.insert({ i, info });

					write.descriptorCount = 1;
					write.descriptorType = atlas_to_vk_descriptor_type(buffer->get_type());
					write.pBufferInfo = &it->second;
				}

				else if (std::holds_alternative<Descriptor::TextureAttachment>(at)) {
					Descriptor::TextureAttachment texture = std::get<Descriptor::TextureAttachment>(at);
					auto info = descriptor_image_info(*texture->get_native_texture());
					auto [it, ex] = imageInfos.insert({ i, info });

					write.descriptorCount = 1;
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.pImageInfo = &it->second;
				}

				else if (std::holds_alternative<Descriptor::TextureArrayAttachment>(at)) {
					Descriptor::TextureArrayAttachment textures = std::get<Descriptor::TextureArrayAttachment>(at);
					std::vector<vkutil::VkTexture> vulkanTextures;
					for (auto &tex : textures) vulkanTextures.push_back(*tex->get_native_texture());
					auto info = descriptor_image_array_info(vulkanTextures.data(), (uint32_t)vulkanTextures.size());
					auto [it, ex] = imageArrayInfos.insert({ i, info });

					write.descriptorCount = (uint32_t)textures.size();
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.pImageInfo = it->second.data();
				}

				writes.push_back(write);
			}

			auto cmd = Application::get_engine().get_active_command_buffer();

			Application::get_engine().vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				shader.get_native_shader()->layout, set, (uint32_t)writes.size(), writes.data());
		}

		VkDescriptor *get_native() {
			return &m_Descriptor;
		}

		uint32_t get_set_count() {
			return (uint32_t)(m_Attachments.size());
		}

	private:
		std::vector<Descriptor::Attachment> m_Attachments;
		bool m_Pushable{ false };
		//std::unordered_map<uint32_t, Ref<Atlas::Buffer>> m_Buffers;
		//std::unordered_map<uint32_t, Ref<Atlas::Texture>> m_Textures;
		//std::unordered_map<uint32_t, std::vector<Ref<Atlas::Texture>>> m_TextureArrays;

		vkutil::VkDescriptor m_Descriptor;
	};

}

namespace Atlas {

	Descriptor::Descriptor(std::vector<Binding> &bindings, bool pushable)
		:m_Initialized(true)
	{
		m_Descriptor = make_ref<vkutil::VulkanDescriptor>(bindings, pushable);
	}

	uint32_t Descriptor::get_set_count()
	{
		if (!m_Initialized) {
			CORE_WARN("Descriptor was not initialized");
			return 0;
		}
		return m_Descriptor->get_set_count();
	}

	void Descriptor::bind(Shader &shader)
	{
		m_Descriptor->bind(shader);
	}

	void Descriptor::push(Shader &shader, uint32_t size)
	{
		m_Descriptor->push(shader, size);
	}

	void Descriptor::update(uint32_t binding, Binding descBinding)
	{
		m_Descriptor->update(binding, descBinding);
	}

	vkutil::VkDescriptor *Descriptor::get_native_descriptor()
	{
		if (!m_Initialized) {
			CORE_WARN("Descriptor was not initialized");
			return nullptr;
		}

		return m_Descriptor->get_native();
	}

}
