#include "renderer.h"
#include "application.h"
#include "shader.h"
#include "buffer.h"
#include "descriptor.h"
#include "camera.h"
#include "orthographic_camera.h"

#include "vk_buffer.h"
#include "vk_engine.h"

#include <glm/gtx/transform.hpp>

namespace Atlas {

	struct GPUCameraData {
		glm::mat4 viewProj;
	};

	struct RenderData {
		bool init{ false };

		uint32_t maxVertices{ 1 };
		uint32_t maxIndices{ 1 };
		//static const uint32_t MAX_VERTICES = 4;
		//static const uint32_t MAX_INDICES = 6;
		static const uint32_t MAX_TEXTURE_SLOTS = 32;

		Shader defaultShader;
		Descriptor defaultDescriptor;
		Ref<Texture> whiteTexture;
		Buffer vertexBuffer;
		Buffer indexBuffer;

		Ref<Buffer> cameraBuffer;
		GPUCameraData camera{};

		std::vector<Render2D::Vertex> vertices;
		std::vector<uint16_t> indices;
		std::vector<Ref<Texture>> textureSlots{};
		//Render2D::Vertex *vertexPtr;
		//uint16_t *indexPtr;
		uint32_t textureSlotIndex = 1;

		Color clearColor{ 255 };
	};

	static RenderData s_Data{};

	void Render2D::begin(Texture &color, Texture &depth)
	{
		s_Data.defaultShader.bind();
		s_Data.vertexBuffer.bind();
		s_Data.indexBuffer.bind();

		RenderApi::begin(color, depth, s_Data.clearColor);
	}

	void Render2D::begin(Texture &color)
	{
		s_Data.defaultShader.bind();
		s_Data.vertexBuffer.bind();
		s_Data.indexBuffer.bind();

		RenderApi::begin(color, s_Data.clearColor);
	}

	void Render2D::end()
	{
		flush();
		RenderApi::end();
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

		//s_Data.vertexPtr = s_Data.vertices.data();
		//s_Data.indexPtr = s_Data.indices.data();

		s_Data.textureSlots.resize(RenderData::MAX_TEXTURE_SLOTS);
		//s_Data.vertices = std::vector<Vertex>(s_Data.vertexCount, Vertex());
		//s_Data.indices = std::vector<uint16_t>(s_Data.indexCount, 0);

		auto vertexDescription = VertexDescription();
		vertexDescription
			.push_attrib(VertexAttribute::FLOAT3, &Vertex::position)
			.push_attrib(VertexAttribute::FLOAT4, &Vertex::color)
			.push_attrib(VertexAttribute::FLOAT2, &Vertex::uv)
			.push_attrib(VertexAttribute::INT, &Vertex::texID)
			.push_attrib(VertexAttribute::FLOAT, &Vertex::sqrRadius);

		GPUCameraData gpuCamera{};
		gpuCamera.viewProj = OrthographicCamera(-1, 1, -1, 1).get_view_projection();

		s_Data.cameraBuffer = make_ref<Buffer>(BufferType::UNIFORM, (uint32_t)sizeof(GPUCameraData));
		s_Data.cameraBuffer->set_data(&gpuCamera, sizeof(GPUCameraData));

		{
			s_Data.whiteTexture = make_ref<Texture>(1, 1, FilterOptions::NEAREST);
			Color data = Color(255);
			s_Data.whiteTexture->set_data((uint32_t *)&data, 1);

			for (auto &tex : s_Data.textureSlots) tex = s_Data.whiteTexture;
		}

		DescriptorBindings bindings = {
			{s_Data.cameraBuffer, ShaderStage::VERTEX},
			{s_Data.textureSlots, ShaderStage::FRAGMENT},
		};

		s_Data.defaultDescriptor = Descriptor(bindings);

		ShaderModule vertModule = load_shader_module("res/shaders/default.vert", ShaderStage::VERTEX, true).value();
		ShaderModule fragModule = load_shader_module("res/shaders/default.frag", ShaderStage::FRAGMENT, true).value();

		ShaderCreateInfo shaderInfo{};
		shaderInfo.modules = { vertModule, fragModule };
		shaderInfo.vertexDescription = vertexDescription;
		shaderInfo.descriptors = { s_Data.defaultDescriptor };

		s_Data.defaultShader = Shader(shaderInfo);

		s_Data.vertexBuffer = Buffer(BufferType::VERTEX, s_Data.maxVertices * sizeof(Vertex));
		s_Data.indexBuffer = Buffer(BufferType::INDEX_U16, s_Data.maxIndices * sizeof(uint16_t));


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
		ATL_EVENT();

		if (s_Data.indices.size() == 0) return;

		if (s_Data.vertices.size() <= s_Data.maxVertices) {
			uint32_t size = (uint32_t)(s_Data.vertices.size() * sizeof(Vertex));
			s_Data.vertexBuffer.set_data(s_Data.vertices.data(), size);
		}
		else {
			s_Data.maxVertices = s_Data.vertices.size() * 1.5;
			s_Data.vertexBuffer = Buffer(BufferType::VERTEX, s_Data.maxVertices * sizeof(Vertex));
			s_Data.vertexBuffer.set_data(s_Data.vertices.data(), s_Data.vertices.size() * sizeof(Vertex));
			s_Data.vertexBuffer.bind();
		}

		if (s_Data.indices.size() <= s_Data.maxIndices) {
			uint32_t size = (uint32_t)(s_Data.indices.size() * sizeof(uint16_t));
			s_Data.indexBuffer.set_data(s_Data.indices.data(), size);
		}
		else {
			s_Data.maxIndices = s_Data.indices.size() * 1.5;
			s_Data.indexBuffer = Buffer(BufferType::INDEX_U16, s_Data.maxIndices * sizeof(uint16_t));
			s_Data.indexBuffer.set_data(s_Data.indices.data(), s_Data.indices.size() * sizeof(uint16_t));
			s_Data.indexBuffer.bind();
		}

		s_Data.defaultDescriptor.update(1, { s_Data.textureSlots, ShaderStage::FRAGMENT });

		RenderApi::drawIndexed(s_Data.indices.size());

		s_Data.textureSlotIndex = 1;
		s_Data.vertices.clear();
		s_Data.indices.clear();
		s_Data.vertices.reserve(s_Data.maxVertices);
		s_Data.vertices.reserve(s_Data.maxIndices);
		//s_Data.vertexPtr = s_Data.vertices.data();
		//s_Data.indexPtr = s_Data.indices.data();
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
		//if (s_Data.vertexCount + 4 > RenderData::MAX_VERTICES) flush();
		//if (s_Data.indexCount + 6 > RenderData::maxIndices) flush();

		glm::vec4 col = color.normalized_vec();

		uint32_t vertOffset = (uint32_t)s_Data.vertices.size();

		Vertex vert{};
		vert.position = glm::vec3(pos, 0.0f);
		vert.color = col;
		vert.uv = { 0, 0 };
		vert.texID = textureIndx;
		vert.sqrRadius = radius;
		s_Data.vertices.push_back(vert);
		//s_Data.vertexPtr++;

		vert.position = { pos.x + size.x, pos.y, 0 };
		vert.color = col;
		vert.uv = { 1, 0 };
		vert.texID = textureIndx;
		vert.sqrRadius = radius;
		s_Data.vertices.push_back(vert);
		//s_Data.vertexPtr++;

		vert.position = { pos.x + size.x, pos.y + size.y, 0 };
		vert.color = col;
		vert.uv = { 1, 1 };
		vert.texID = textureIndx;
		vert.sqrRadius = radius;
		s_Data.vertices.push_back(vert);
		//s_Data.vertexPtr++;

		vert.position = { pos.x, pos.y + size.y, 0 };
		vert.color = col;
		vert.uv = { 0, 1 };
		vert.texID = textureIndx;
		vert.sqrRadius = radius;
		s_Data.vertices.push_back(vert);
		//s_Data.vertexPtr++;

		s_Data.indices.push_back((uint16_t)(vertOffset + 0));
		s_Data.indices.push_back((uint16_t)(vertOffset + 1));
		s_Data.indices.push_back((uint16_t)(vertOffset + 2));
		s_Data.indices.push_back((uint16_t)(vertOffset + 2));
		s_Data.indices.push_back((uint16_t)(vertOffset + 3));
		s_Data.indices.push_back((uint16_t)(vertOffset + 0));
		//*s_Data.indexPtr++ = s_Data.vertexCount + 0;
		//*s_Data.indexPtr++ = s_Data.vertexCount + 1;
		//*s_Data.indexPtr++ = s_Data.vertexCount + 2;
		//*s_Data.indexPtr++ = s_Data.vertexCount + 2;
		//*s_Data.indexPtr++ = s_Data.vertexCount + 3;
		//*s_Data.indexPtr++ = s_Data.vertexCount + 0;
	}

	void Render2D::rect(const glm::vec2 &pos, const glm::vec2 &size, Color color)
	{
		rect(pos, size, color, 0, 2);
	}

	void Render2D::rect(const glm::vec2 &pos, const glm::vec2 &size, Ref<Texture> texture)
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

		rect(pos, size, { 255 }, textureIndx, 2);
	}

	void Render2D::round_rect(const glm::vec2 &pos, const glm::vec2 &size, float radius, Color color)
	{
		rect(pos, size, color, 0, radius);
	}

	void Render2D::round_rect(const glm::vec2 &pos, const glm::vec2 &size, float radius, Ref<Texture> texture)
	{
		rect(pos, size, { 255 }, 0, radius);
	}

	void Render2D::circle(const glm::vec2 &pos, const float radius, Color color)
	{
		glm::vec2 size = { radius, radius };
		rect(pos - size, glm::vec2(radius * 2, radius * 2), color, 0, 1);
	}

}

