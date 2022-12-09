#include "renderer.h"
#include "application.h"
#include "shader.h"
#include "buffer.h"
#include "descriptor.h"
#include "camera.h"
#include "orthographic_camera.h"

#include <glm/gtx/transform.hpp>

namespace Atlas {

	struct GPUCameraData {
		glm::mat4 viewProj;
	};

	struct RenderData {
		bool init{ false };

		Shader defaultShader;

		Buffer vertexBuffer;
		Buffer indexBuffer;

		Ref<Buffer> cameraBuffer;
		GPUCameraData camera{};

		Ref<Texture> whiteTexture;

		std::vector<Render2D::Vertex> vertices;
		std::vector<uint16_t> indices;
	};

	static RenderData s_Data;

	void Render2D::init()
	{
		if (s_Data.init) {
			CORE_WARN("Renderer already initialized!");
			return;
		}

		ATL_EVENT();

		s_Data.init = true;

		{
			auto vertexDescription = VertexDescription();
			vertexDescription
				.push_attrib(VertexAttribute::FLOAT3, &Vertex::position)
				.push_attrib(VertexAttribute::FLOAT4, &Vertex::color)
				.push_attrib(VertexAttribute::FLOAT2, &Vertex::uv);

			GPUCameraData gpuCamera{};
			gpuCamera.viewProj = OrthographicCamera(-1, 1, -1, 1).get_view_projection();

			s_Data.cameraBuffer = make_ref<Buffer>(BufferType::UNIFORM, (uint32_t)sizeof(GPUCameraData));
			s_Data.cameraBuffer->set_data(&gpuCamera, sizeof(GPUCameraData));

			Ref<Texture> tex1 = make_ref<Texture>("res/images/uv_checker_v1.png", FilterOptions::NEAREST);
			Ref<Texture> tex2 = make_ref<Texture>("res/images/uv_checker_v2.png", FilterOptions::NEAREST);

			{
				s_Data.whiteTexture = make_ref<Texture>(1, 1, FilterOptions::NEAREST);
				Color data = Color(255, 0, 0);
				s_Data.whiteTexture->set_data(&data, 1);
			}

			std::vector<Ref<Texture>> textureArr = { tex1, tex2 };

			DescriptorBindings bindings = {
				{s_Data.cameraBuffer, ShaderStage::VERTEX},
				{s_Data.whiteTexture, ShaderStage::FRAGMENT},
			};

			Descriptor descriptorSet = Descriptor(bindings);

			ShaderModule vertModule = load_shader_module("res/shaders/default.vert", ShaderStage::VERTEX, true).value();
			ShaderModule fragModule = load_shader_module("res/shaders/default.frag", ShaderStage::FRAGMENT, true).value();

			ShaderCreateInfo shaderInfo{};
			shaderInfo.modules = { vertModule, fragModule };
			shaderInfo.vertexDescription = vertexDescription;
			shaderInfo.descriptors = { descriptorSet };

			s_Data.defaultShader = Shader(shaderInfo);
		}

		{
			//std::array<Vertex, 4> vertices{};
			//vertices[0].position = glm::vec3(1, 0, 0);
			//vertices[0].color = glm::vec4(1, 1, 1, 1);
			//vertices[0].uv = glm::vec2(1, 0);

			//vertices[1].position = glm::vec3(0, 1, 0);
			//vertices[1].color = glm::vec4(1, 1, 1, 1);
			//vertices[1].uv = glm::vec2(0, 1);

			//vertices[2].position = glm::vec3(0, 0, 0);
			//vertices[2].color = glm::vec4(1, 1, 1, 1);
			//vertices[2].uv = glm::vec2(0, 0);

			//vertices[3].position = glm::vec3(1, 1, 0);
			//vertices[3].color = glm::vec4(1, 1, 1, 1);
			//vertices[3].uv = glm::vec2(1, 1);

			//s_Data.vertexBuffer = Buffer(BufferType::VERTEX, vertices.data(), (uint32_t)(vertices.size() * sizeof(Vertex)));

			//std::array<uint16_t, 6> indices = { 0, 1, 2, 0, 3, 1 };
			//s_Data.indexBuffer = Buffer(BufferType::INDEX_U16, indices.data(), (uint32_t)(indices.size() * sizeof(uint16_t)));

			Vertex vert{};
			vert.position = glm::vec3(1, 0, 0);
			vert.color = glm::vec4(1, 1, 1, 1);
			vert.uv = glm::vec2(1, 0);
			s_Data.vertices.push_back(vert);

			vert.position = glm::vec3(0, 1, 0);
			vert.color = glm::vec4(1, 1, 1, 1);
			vert.uv = glm::vec2(0, 1);
			s_Data.vertices.push_back(vert);

			vert.position = glm::vec3(0, 0, 0);
			vert.color = glm::vec4(1, 1, 1, 1);
			vert.uv = glm::vec2(0, 0);
			s_Data.vertices.push_back(vert);

			vert.position = glm::vec3(1, 1, 0);
			vert.color = glm::vec4(1, 1, 1, 1);
			vert.uv = glm::vec2(1, 1);
			s_Data.vertices.push_back(vert);

			s_Data.indices.insert(s_Data.indices.end(), { 0, 1, 2, 0, 3, 1 });
		}
	}

	void Render2D::cleanup()
	{
		if (!s_Data.init) {
			CORE_WARN("Renderer not initialized!");
			return;
		}

	}

	void Render2D::set_camera(Camera &camera)
	{
		if (s_Data.camera.viewProj != camera.get_view_projection()) {
			s_Data.camera.viewProj = camera.get_view_projection();
			s_Data.cameraBuffer->set_data(&s_Data.camera, sizeof(GPUCameraData));
		}
	}

	void Render2D::draw_test_triangle()
	{
		OPTICK_EVENT();

		auto vertexBuffer = Buffer(BufferType::VERTEX, s_Data.vertices.data(), s_Data.vertices.size() * sizeof(Vertex), true);
		auto indexBuffer = Buffer(BufferType::INDEX_U16, s_Data.indices.data(), s_Data.indices.size() * sizeof(uint16_t), true);

		s_Data.defaultShader.bind();
		vertexBuffer.bind();
		indexBuffer.bind();

		RenderApi::drawIndexed(6, 1);
	}

}

