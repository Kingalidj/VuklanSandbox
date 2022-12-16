#include "renderer.h"
#include "application.h"
#include "shader.h"
#include "buffer.h"
#include "descriptor.h"
#include "camera.h"
#include "orthographic_camera.h"

#include "vk_engine.h"

#include <glm/gtx/transform.hpp>

namespace Atlas {

	struct GPUCameraData {
		glm::mat4 viewProj;
	};

	struct RenderData {
		bool init{ false };

		//const uint32_t maxVertices{ 500 };
		//const uint32_t maxIndices{ 300 };
		static const uint32_t MAX_VERTICES = 6000;
		static const uint32_t MAX_INDICES = 100000;
		static const uint32_t MAX_TEXTURE_SLOTS = 25;

		Shader defaultShader;
		Descriptor defaultDescriptor;
		Ref<Texture> whiteTexture;
		Buffer vertexBuffer;
		Buffer indexBuffer;

		Ref<Buffer> cameraBuffer;
		GPUCameraData camera{};

		std::array<Render2D::Vertex, MAX_VERTICES> vertices{ Render2D::Vertex() };
		std::array<uint32_t, MAX_INDICES> indices{ 0 };
		std::vector<Ref<Texture>> textureSlots{};

		Render2D::Vertex *vertexPtr;
		uint32_t vertexCount{ 0 };
		uint32_t indexCount{ 0 };
		uint32_t *indexPtr;
		uint32_t textureSlotIndex = 1;

		Color clearColor{ 255 };

		Ref<Texture> renderColorTarget{};
		Ref<Texture> renderDepthTarget{};
	};

	static RenderData s_Data{};
	const uint32_t RenderData::MAX_TEXTURE_SLOTS;

	void Render2D::begin(Ref<Texture> color, Ref<Texture> depth)
	{
		s_Data.renderColorTarget = color;
		s_Data.renderDepthTarget = depth;

		s_Data.defaultShader.bind();
		s_Data.vertexBuffer.bind();
		s_Data.indexBuffer.bind();
		//s_Data.defaultDescriptor.push(s_Data.defaultShader, 0);

		RenderApi::begin(color, depth, s_Data.clearColor);
	}

	void Render2D::begin(Ref<Texture> color)
	{
		s_Data.renderColorTarget = color;

		s_Data.defaultShader.bind();
		s_Data.vertexBuffer.bind();
		s_Data.indexBuffer.bind();


		RenderApi::begin(color, s_Data.clearColor);
	}

	void Render2D::end()
	{
		if (!s_Data.renderColorTarget) {
			CORE_WARN("Render2D::begin was never called!");
		}

		flush();

		RenderApi::end();

		//vkutil::full_pipeline_barrier(Application::get_engine().get_active_command_buffer());

		s_Data.renderColorTarget = nullptr;
		s_Data.renderDepthTarget = nullptr;
	}

	void Render2D::clear_color(Color color) {
		s_Data.clearColor = color;
	}

	void Render2D::init()
	{
		if (s_Data.init) {
			CORE_WARN("Renderer already initialized!");
			return;
		}

		ATL_EVENT();

		s_Data.init = true;

		s_Data.vertexPtr = s_Data.vertices.data();
		s_Data.indexPtr = s_Data.indices.data();

		s_Data.textureSlots.resize(RenderData::MAX_TEXTURE_SLOTS);
		//s_Data.vertices = std::vector<Vertex>(s_Data.vertexCount, Vertex());
		//s_Data.indices = std::vector<uint16_t>(s_Data.indexCount, 0);

		auto vertexDescription = VertexDescription();
		vertexDescription
			.push_attrib(VertexAttribute::FLOAT2, &Vertex::position)
			.push_attrib(VertexAttribute::FLOAT4, &Vertex::color)
			.push_attrib(VertexAttribute::FLOAT2, &Vertex::uv)
			.push_attrib(VertexAttribute::INT, &Vertex::texID)
			.push_attrib(VertexAttribute::FLOAT, &Vertex::sqrRadius);

		GPUCameraData gpuCamera{};
		gpuCamera.viewProj = OrthographicCamera(-1, 1, -1, 1).get_view_projection();

		s_Data.cameraBuffer = make_ref<Buffer>();
		*s_Data.cameraBuffer = Buffer::uniform((uint32_t)sizeof(gpuCamera), true);
		s_Data.cameraBuffer->set_data(&gpuCamera, sizeof(GPUCameraData));

		{
			s_Data.whiteTexture = make_ref<Texture>(1, 1, FilterOptions::NEAREST);
			Color data = Color(255);
			s_Data.whiteTexture->set_data((uint32_t *)&data, 1);

			for (auto &tex : s_Data.textureSlots) tex = s_Data.whiteTexture;
		}

		Descriptor::Bindings bindings = {
			{s_Data.cameraBuffer, ShaderStage::VERTEX},
			{s_Data.textureSlots, ShaderStage::FRAGMENT},
		};

		s_Data.defaultDescriptor = Descriptor(bindings, true);

		ShaderModule vertModule = ShaderModule::load("res/shaders/default.vert", ShaderStage::VERTEX, true).value();
		ShaderModule fragModule = ShaderModule::load("res/shaders/default.frag", ShaderStage::FRAGMENT, true).value();

		ShaderCreateInfo shaderInfo{};
		shaderInfo.modules = { vertModule, fragModule };
		shaderInfo.vertexDescription = vertexDescription;
		shaderInfo.descriptors = { s_Data.defaultDescriptor };

		s_Data.defaultShader = Shader(shaderInfo);

		//s_Data.vertexBuffer = Buffer::vertex(uint32_t(s_Data.maxVertices * sizeof(Vertex)));
		//s_Data.indexBuffer = Buffer::index_u32(uint32_t(s_Data.maxIndices * sizeof(uint32_t)));
		s_Data.vertexBuffer = Buffer::vertex(uint32_t(RenderData::MAX_VERTICES * sizeof(Vertex)), true);
		s_Data.indexBuffer = Buffer::index_u32(uint32_t(RenderData::MAX_INDICES * sizeof(uint32_t)), true);

	}

	void Render2D::cleanup()
	{
		if (!s_Data.init) {
			CORE_WARN("Renderer not initialized!");
			return;
		}

	}

	void Render2D::flush()
	{
		if (!s_Data.renderColorTarget) {
			CORE_WARN("Render2D::begin was never called!");
		}

		ATL_EVENT();

		if (s_Data.indexCount == 0) return;

		RenderApi::end();

		s_Data.vertexBuffer.set_data(s_Data.vertices.data(), s_Data.vertexCount * sizeof(Vertex));
		s_Data.indexBuffer.set_data(s_Data.indices.data(), s_Data.indexCount * sizeof(uint32_t));

		RenderApi::begin(s_Data.renderColorTarget, { 0, 0, 0, 0 });


		s_Data.vertexBuffer.bind();
		s_Data.indexBuffer.bind();

		s_Data.defaultDescriptor.update(1, { s_Data.textureSlots, ShaderStage::FRAGMENT });
		s_Data.defaultDescriptor.push(s_Data.defaultShader, 0);

		//RenderApi::end();
		//RenderApi::begin(s_Data.renderColorTarget, { 0, 0, 0, 0 });
		//if (s_Data.renderDepthTarget)
		//	RenderApi::begin(s_Data.renderColorTarget, s_Data.renderDepthTarget, { 0, 0, 0, 0 });
		//else
		//	RenderApi::begin(s_Data.renderColorTarget, { 0, 0, 0, 0 });

		RenderApi::drawIndexed(s_Data.indexCount);


		s_Data.textureSlotIndex = 1;
		s_Data.vertexPtr = s_Data.vertices.data();
		s_Data.indexPtr = s_Data.indices.data();
		s_Data.indexCount = 0;
		s_Data.vertexCount = 0;
	}

	void Render2D::set_camera(Camera &camera)
	{
		if (s_Data.camera.viewProj != camera.get_view_projection()) {
			s_Data.camera.viewProj = camera.get_view_projection();
			s_Data.cameraBuffer->set_data(&s_Data.camera, sizeof(GPUCameraData));
		}
	}

	void Render2D::rect(const glm::vec2 &pos, const glm::vec2 &size, Color color, uint32_t textureIndx, float radius)
	{
		//if (s_Data.vertexCount + 4 >= RenderData::MAX_VERTICES) flush();
		//if (s_Data.indexCount + 6 >= RenderData::MAX_INDICES) flush();

		glm::vec4 col = color.normalized_vec();

		s_Data.vertexPtr->position = glm::vec3(pos, 0.0f);
		s_Data.vertexPtr->color = col;
		s_Data.vertexPtr->uv = { 0, 0 };
		s_Data.vertexPtr->texID = textureIndx;
		s_Data.vertexPtr->sqrRadius = radius;
		s_Data.vertexPtr++;
		//s_Data.vertices.push_back(vert);

		s_Data.vertexPtr->position = { pos.x + size.x, pos.y };
		s_Data.vertexPtr->color = col;
		s_Data.vertexPtr->uv = { 1, 0 };
		s_Data.vertexPtr->texID = textureIndx;
		s_Data.vertexPtr->sqrRadius = radius;
		s_Data.vertexPtr++;
		//s_Data.vertices.push_back(vert);

		s_Data.vertexPtr->position = { pos.x + size.x, pos.y + size.y };
		s_Data.vertexPtr->color = col;
		s_Data.vertexPtr->uv = { 1, 1 };
		s_Data.vertexPtr->texID = textureIndx;
		s_Data.vertexPtr->sqrRadius = radius;
		s_Data.vertexPtr++;
		//s_Data.vertices.push_back(vert);

		s_Data.vertexPtr->position = { pos.x, pos.y + size.y };
		s_Data.vertexPtr->color = col;
		s_Data.vertexPtr->uv = { 0, 1 };
		s_Data.vertexPtr->texID = textureIndx;
		s_Data.vertexPtr->sqrRadius = radius;
		s_Data.vertexPtr++;
		//s_Data.vertices.push_back(vert);

		*(s_Data.indexPtr++) = s_Data.vertexCount + 0;
		*(s_Data.indexPtr++) = s_Data.vertexCount + 1;
		*(s_Data.indexPtr++) = s_Data.vertexCount + 2;
		*(s_Data.indexPtr++) = s_Data.vertexCount + 2;
		*(s_Data.indexPtr++) = s_Data.vertexCount + 3;
		*(s_Data.indexPtr++) = s_Data.vertexCount + 0;
		//s_Data.indices.push_back((uint32_t)(vertOffset + 0));
		//s_Data.indices.push_back((uint32_t)(vertOffset + 1));
		//s_Data.indices.push_back((uint32_t)(vertOffset + 2));
		//s_Data.indices.push_back((uint32_t)(vertOffset + 2));
		//s_Data.indices.push_back((uint32_t)(vertOffset + 3));
		//s_Data.indices.push_back((uint32_t)(vertOffset + 0));

		s_Data.vertexCount += 4;
		s_Data.indexCount += 6;
	}

	void Render2D::rect(const glm::vec2 &pos, const glm::vec2 &size, Color color)
	{
		rect(pos, size, color, 0, 2);
	}

	void Render2D::rect(const glm::vec2 &pos, const glm::vec2 &size, Ref<Texture> texture, Color tint)
	{

		int textureIndx = 0;

		for (uint32_t i = 1; i < s_Data.textureSlotIndex; i++) {
			if (s_Data.textureSlots.at(i) == texture) {
				textureIndx = i;
				break;
			}
		}

		if (textureIndx == 0 && s_Data.textureSlotIndex + 1 > RenderData::MAX_TEXTURE_SLOTS) {
			CORE_WARN("Render2D: not more than {} texture slots supported!", RenderData::MAX_TEXTURE_SLOTS);
			return;
		}

		if (textureIndx == 0) {
			textureIndx = s_Data.textureSlotIndex++;
			s_Data.textureSlots[textureIndx] = texture;
		}

		rect(pos, size, tint, textureIndx, 2);
	}

	void Render2D::circle(const glm::vec2 &pos, const float radius, Color color)
	{
		glm::vec2 size = { radius, radius };
		rect(pos - size, glm::vec2(radius * 2, radius * 2), color, 0, 1);
	}

	void Render2D::test_render(Ref<Texture> colorTex)
	{
		glm::vec2 pos = { 0, 0 };

		Color color(200, 0, 0);
		glm::vec4 col = color.normalized_vec();

		std::array<Vertex, 3> verts{};
		std::array<uint32_t, 3> indices{};

		verts[0].position = glm::vec3(pos, 0.0f);
		verts[0].color = col;
		verts[0].uv = { 0, 0 };
		verts[0].texID = 0;
		verts[0].sqrRadius = 2;

		verts[1].position = { pos.x + 1, pos.y };
		verts[1].color = col;
		verts[1].uv = { 1, 0 };
		verts[1].texID = 0;
		verts[1].sqrRadius = 2;

		verts[2].position = { pos.x + 1, pos.y + 1 };
		verts[2].color = col;
		verts[2].uv = { 1, 1 };
		verts[2].texID = 0;
		verts[2].sqrRadius = 2;

		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;

		s_Data.vertexBuffer.set_data(verts.data(), 3 * sizeof(Vertex));
		s_Data.indexBuffer.set_data(indices.data(), 3 * sizeof(uint32_t));

		RenderApi::begin(colorTex, { 255 });

		s_Data.defaultShader.bind();
		s_Data.defaultDescriptor.push(s_Data.defaultShader, 0);
		s_Data.vertexBuffer.bind();
		s_Data.indexBuffer.bind();

		RenderApi::drawIndexed(3);
		RenderApi::end();

		vkutil::full_pipeline_barrier(Application::get_engine().get_active_command_buffer());

		pos = { 2, 2 };

		verts[0].position = glm::vec3(pos, 0.0f);
		verts[0].color = col;
		verts[0].uv = { 0, 0 };
		verts[0].texID = 0;
		verts[0].sqrRadius = 2;

		verts[1].position = { pos.x + 1, pos.y };
		verts[1].color = col;
		verts[1].uv = { 1, 0 };
		verts[1].texID = 0;
		verts[1].sqrRadius = 2;

		verts[2].position = { pos.x + 1, pos.y + 1 };
		verts[2].color = col;
		verts[2].uv = { 1, 1 };
		verts[2].texID = 0;
		verts[2].sqrRadius = 2;

		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;

		s_Data.vertexBuffer.set_data(verts.data(), 3 * sizeof(Vertex));
		s_Data.indexBuffer.set_data(indices.data(), 3 * sizeof(uint32_t));

		RenderApi::begin(colorTex, { 0, 0, 0, 0 });

		s_Data.defaultShader.bind();
		s_Data.defaultDescriptor.push(s_Data.defaultShader, 0);
		s_Data.vertexBuffer.bind();
		s_Data.indexBuffer.bind();

		RenderApi::drawIndexed(3);
		RenderApi::end();

	}

}

