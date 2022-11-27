#include "renderer.h"
#include "application.h"
#include "shader.h"

#include "vk_scene.h"
#include "vk_pipeline.h"
#include "vk_engine.h"

namespace Atlas {

	struct RenderData {
		bool init{ false };

		VertexDescription vertexDescription;
		Shader defaultShader;

		std::vector<Render2D::Vertex> triangleVertices;
		vkutil::AllocatedBuffer triangleVertexBuffer;
	};

	static RenderData s_Data;

	void Render2D::init()
	{
		if (s_Data.init) {
			CORE_WARN("Renderer already initialized!");
			return;
		}

		s_Data.init = true;

		vkutil::VulkanEngine &engine = Application::get_engine();
		vkutil::VulkanManager &manager = engine.manager();
		VkDevice device = manager.device();

		s_Data.vertexDescription = VertexDescription()
			.push_attrib(VertexAttribute::FLOAT3, &Vertex::position)
			.push_attrib(VertexAttribute::FLOAT4, &Vertex::color)
			.push_attrib(VertexAttribute::FLOAT2, &Vertex::uv);

		s_Data.defaultShader = Shader(s_Data.vertexDescription,
			{
				"res/shaders/default.vert",
				"res/shaders/default.frag",
			});

		s_Data.triangleVertices.resize(3);
		s_Data.triangleVertices[0].position = glm::vec3(1, 0, 0);
		s_Data.triangleVertices[0].color = glm::vec4(1, 0, 0, 1);

		s_Data.triangleVertices[1].position = glm::vec3(0, 1, 0);
		s_Data.triangleVertices[1].color = glm::vec4(0, 1, 0, 1);

		s_Data.triangleVertices[2].position = glm::vec3(0, 0, 1);
		s_Data.triangleVertices[2].color = glm::vec4(1, 0, 1, 1);

		Application::get_engine().manager().upload_to_gpu(
			s_Data.triangleVertices.data(), (uint32_t)s_Data.triangleVertices.size() * sizeof(Vertex),
			s_Data.triangleVertexBuffer,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	void Render2D::cleanup()
	{
		if (!s_Data.init) {
			CORE_WARN("Renderer not initialized!");
			return;
		}

		vmaDestroyBuffer(Application::get_engine().manager().get_allocator(), s_Data.triangleVertexBuffer.buffer, s_Data.triangleVertexBuffer.allocation);
	}

	void Render2D::draw_test_triangle()
	{
		VkCommandBuffer cmd = Application::get_engine().get_active_command_buffer();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.defaultShader.get_native_shader()->pipeline);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &s_Data.triangleVertexBuffer.buffer, &offset);

		vkCmdDraw(cmd, s_Data.triangleVertices.size(), 1, 0, 0);
	}

}

