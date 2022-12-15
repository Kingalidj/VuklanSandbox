#include "atl_vk_utils.h"
#include "application.h"
#include "vk_engine.h"

namespace Atlas {

	VkDescriptorType atlas_to_vk_descriptor_type(BufferTypeFlags type) {

		if (type & BufferType::UNIFORM) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		if (type & BufferType::STORAGE) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		CORE_ASSERT(false, "neither uniform or storage flag set for buffer");
	}

	VkShaderStageFlagBits atlas_to_vk_shaderstage(ShaderStage type) {
		switch (type)
		{
		case ShaderStage::VERTEX:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::FRAGMENT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::COMPUTE:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		default:
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}
	}

	vkutil::VertexAttributeType atlas_to_vk_attribute(VertexAttribute &attribute) {
		switch (attribute) {
		case VertexAttribute::INT: return vkutil::VertexAttributeType::INT;
		case VertexAttribute::INT2: return vkutil::VertexAttributeType::INT2;
		case VertexAttribute::INT3: return vkutil::VertexAttributeType::INT3;
		case VertexAttribute::INT4: return vkutil::VertexAttributeType::INT4;
		case VertexAttribute::FLOAT: return vkutil::VertexAttributeType::FLOAT;
		case VertexAttribute::FLOAT2: return vkutil::VertexAttributeType::FLOAT2;
		case VertexAttribute::FLOAT3: return vkutil::VertexAttributeType::FLOAT3;
		case VertexAttribute::FLOAT4: return vkutil::VertexAttributeType::FLOAT4;
		default: CORE_ASSERT(false, "never called");
		}

		return vkutil::VertexAttributeType::INT;
	}

	VkFilter atlas_to_vk_filter(FilterOptions options) {

		switch (options) {
		case FilterOptions::LINEAR:
			return VK_FILTER_LINEAR;
		case FilterOptions::NEAREST:
			return VK_FILTER_NEAREST;
		default:
			CORE_ASSERT(false, "never called");
		}

		return VK_FILTER_MAX_ENUM;
	}

	vkutil::TextureCreateInfo color_format_to_texture_info(TextureFormat f, uint32_t w, uint32_t h) {
		VkFormat format{};

		switch (f) {
		case TextureFormat::R8G8B8A8:
			format = Atlas::Application::get_engine().get_color_format();
			return vkutil::color_texture_create_info(w, h, format);
		case TextureFormat::D32:
			format = Atlas::Application::get_engine().get_depth_format();
			return vkutil::depth_texture_create_info(w, h, format);
		default:
			CORE_ASSERT(false, "undefined color format");
		}

		return vkutil::TextureCreateInfo{};
	}

}
