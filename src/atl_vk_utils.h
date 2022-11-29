#pragma once

#include "vk_types.h"
#include "vk_scene.h"
#include "vk_textures.h"

#include "buffer.h"
#include "shader.h"
#include "texture.h"

namespace Atlas {

	VkDescriptorType atlas_to_vk_descriptor_type(BufferType type);
	VkShaderStageFlagBits atlas_to_vk_shaderstage(ShaderStage type);
	vkutil::VertexAttributeType atlas_to_vk_attribute(VertexAttribute &attribute);
	VkFilter atlas_to_vk_filter(FilterOptions options);
	vkutil::TextureCreateInfo color_format_to_texture_info(ColorFormat f, uint32_t w, uint32_t h);



}
