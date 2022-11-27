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

	enum class VertexAttributeType : int {
		INT = VK_FORMAT_R32_SINT,
		INT2 = VK_FORMAT_R32G32_SINT,
		INT3 = VK_FORMAT_R32G32B32_SINT,
		INT4 = VK_FORMAT_R32G32B32A32_SINT,
		FLOAT = VK_FORMAT_R32_SFLOAT,
		FLOAT2 = VK_FORMAT_R32G32_SFLOAT,
		FLOAT3 = VK_FORMAT_R32G32B32_SFLOAT,
		FLOAT4 = VK_FORMAT_R32G32B32A32_SFLOAT,
	};

	class VertexInputDescriptionBuilder {
	public:

		VertexInputDescriptionBuilder::VertexInputDescriptionBuilder(uint32_t size)
		{
			VkVertexInputBindingDescription mainBinding{};
			mainBinding.binding = 0;
			mainBinding.stride = size;
			mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			m_Description.bindings.push_back(mainBinding);
		}

		inline VertexInputDescription value() { return m_Description; }

		template <typename T, typename U>
		VertexInputDescriptionBuilder &push_attrib(VertexAttributeType type, U T:: *member) {

			VkVertexInputAttributeDescription attribute{};
			attribute.binding = 0;
			attribute.location = m_AttribLocation++;
			attribute.format = (VkFormat)type;
			attribute.offset = (uint32_t)offset_of(member);

			m_Description.attributes.push_back(attribute);
			return *this;
		}

		VertexInputDescriptionBuilder &push_attrib(VertexAttributeType type, uint32_t offset) {

			VkVertexInputAttributeDescription attribute{};
			attribute.binding = 0;
			attribute.location = m_AttribLocation++;
			attribute.format = (VkFormat)type;
			attribute.offset = offset;

			m_Description.attributes.push_back(attribute);
			return *this;
		}

	private:

		VertexInputDescription m_Description;
		uint32_t m_AttribLocation = 0;

		template<typename T, typename U> constexpr size_t offset_of(U T:: *member)
		{
			return (char *)&((T *)nullptr->*member) - (char *)nullptr;
		}

	};

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 color;
		glm::vec2 uv;

		//static vkutil::VertexInputDescription get_vertex_description();
	};

	struct Mesh {
		std::vector<Vertex> vertices;
		AllocatedBuffer vertexBuffer;
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

}
