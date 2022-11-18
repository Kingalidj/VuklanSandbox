#pragma once

#include "vk_types.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace vkutil {

	struct VertexInputDescription {
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;

		VkPipelineVertexInputStateCreateFlags flags = 0;
	};
}

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;

	static vkutil::VertexInputDescription get_vertex_description();
};

struct Mesh {
	std::vector<Vertex> vertices;
	vkutil::AllocatedBuffer vertexBuffer;
};

std::optional<Ref<Mesh>> load_mesh_from_obj(const char *filename);

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSet textureSet{ VK_NULL_HANDLE };
};

struct RenderObject {
	Ref<Mesh> mesh;
	Ref<Material> material;
	glm::mat4 transformMatrix;
};
