#include "renderer.h"
#include "application.h"
#include "shader.h"
#include "buffer.h"
#include "descriptor.h"
#include "camera.h"

#include "vk_scene.h"
#include "vk_pipeline.h"
#include "vk_engine.h"

#include <glm/gtx/transform.hpp>

namespace Atlas {

	struct GPUCameraData {
		glm::mat4 viewProj;
	};

	struct RenderData {
		bool init{ false };

		Shader defaultShader;

		Buffer triangleVertexBuffer;
		Buffer triangleIndexBuffer;

		Ref<Buffer> cameraBuffer;
		GPUCameraData camera;
	};

	static RenderData s_Data;

	void Render2D::init()
	{
		if (s_Data.init) {
			CORE_WARN("Renderer already initialized!");
			return;
		}

		s_Data.init = true;

		{
			auto vertexDescription = VertexDescription();
			vertexDescription
				.push_attrib(VertexAttribute::FLOAT3, &Vertex::position)
				.push_attrib(VertexAttribute::FLOAT4, &Vertex::color)
				.push_attrib(VertexAttribute::FLOAT2, &Vertex::uv);

			s_Data.cameraBuffer = make_ref<Buffer>(BufferType::UNIFORM, sizeof(GPUCameraData));

			Ref<Texture> tex1 = make_ref<Texture>("res/images/uv_checker_v1.png", FilterOptions::NEAREST);
			Ref<Texture> tex2 = make_ref<Texture>("res/images/uv_checker_v2.png", FilterOptions::NEAREST);

			std::vector<Ref<Texture>> textureArr = { tex1, tex2 };

			DescriptorCreateInfo descInfo;
			descInfo.bindings = {
				{s_Data.cameraBuffer, ShaderStage::VERTEX},
				{tex1, ShaderStage::FRAGMENT},
			};

			Ref<Descriptor> descriptorSet = make_ref<Descriptor>(descInfo);

			ShaderModule vertModule = ShaderModule("res/shaders/default.vert.spv", ShaderStage::VERTEX);
			ShaderModule fragModule = ShaderModule("res/shaders/default.frag.spv", ShaderStage::FRAGMENT);

			ShaderCreateInfo shaderInfo{};
			shaderInfo.modules = { &vertModule, &fragModule };
			shaderInfo.vertexDescription = vertexDescription;
			shaderInfo.descriptors = { descriptorSet };

			s_Data.defaultShader = Shader(shaderInfo);
		}

		{
			std::array<Vertex, 4> vertices{};
			vertices[0].position = glm::vec3(1, 0, 0);
			vertices[0].color = glm::vec4(1, 0, 0, 1);
			vertices[0].uv = glm::vec2(1, 0);

			vertices[1].position = glm::vec3(0, 1, 0);
			vertices[1].color = glm::vec4(0, 1, 0, 1);
			vertices[1].uv = glm::vec2(0, 1);

			vertices[2].position = glm::vec3(0, 0, -1);
			vertices[2].color = glm::vec4(0, 0, 1, 1);
			vertices[2].uv = glm::vec2(0, 0);

			vertices[3].position = glm::vec3(1, 1, 0);
			vertices[3].color = glm::vec4(0, 0, 0, 1);
			vertices[3].uv = glm::vec2(1, 1);

			s_Data.triangleVertexBuffer = Buffer(BufferType::VERTEX, vertices.data(), vertices.size() * sizeof(Vertex));

			std::array<uint16_t, 6> indices = { 0, 1, 2, 0, 3, 1 };
			s_Data.triangleIndexBuffer = Buffer(BufferType::INDEX_U16, indices.data(), indices.size() * sizeof(uint16_t));
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

		s_Data.defaultShader.bind();
		s_Data.triangleVertexBuffer.bind();
		s_Data.triangleIndexBuffer.bind();

		VkCommandBuffer cmd = Application::get_engine().get_active_command_buffer();
		vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
	}

}

