#include "shader.h"
#include "application.h"

#include "vk_pipeline.h"
#include "vk_engine.h"

namespace Atlas {

	VkShaderStageFlagBits atlas_to_vk_shadertype(ShaderType type) {
		switch (type)
		{
		case Atlas::ShaderType::VERTEX:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case Atlas::ShaderType::FRAGMENT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
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
	}


	Shader::Shader(VertexDescription &desc, std::initializer_list<std::pair<const char *, ShaderType>> shaders)
	{
		vkutil::VulkanEngine &engine = Application::get_engine();
		vkutil::VulkanManager &manager = engine.manager();



		vkutil::VertexInputDescriptionBuilder vertexBuilder(desc.get_stride());
		for (auto &pair : desc.get_attributes()) {
			vertexBuilder.push_attrib(atlas_to_vk_attribute(pair.first), pair.second);
		}

		vkutil::VertexInputDescription vertexInputDescription = vertexBuilder.value();

		vkutil::PipelineBuilder builder(manager);
		builder.set_vertex_description(vertexInputDescription)
			.set_color_format(engine.get_color_format())
			.set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL, engine.get_depth_format());

		std::vector<VkShaderModule> modules;

		for (auto &pair : shaders) {
			std::filesystem::path file(pair.first);

			if (!std::filesystem::exists(file)) {
				CORE_WARN("Shader: could not find file: {}", file);
				continue;
			}

			VkShaderStageFlagBits shaderType = atlas_to_vk_shadertype(pair.second);

			bool success = false;

			VkShaderModule module{};
			if (file.extension() == ".spv")
				success = vkutil::load_spirv_shader_module(pair.first, &module, manager.device());
			else
				success = vkutil::load_glsl_shader_module(manager, file, shaderType, &module);

			if (!success) {
				CORE_WARN("Shader: unknown error: {}", file);
				continue;
			}

			builder.add_shader_module(module, shaderType);

			modules.push_back(module);
		}

		//m_Shader = make_scope<vkutil::Shader>();
		vkutil::Shader shader{};
		builder.build(&shader);

		for (auto &module : modules) vkDestroyShaderModule(manager.device(), module, nullptr);

		m_Shader = engine.asset_manager().register_shader(shader);
	}

	Shader::Shader(VertexDescription &desc, std::initializer_list<const char *> shaders)
	{
		vkutil::VulkanEngine &engine = Application::get_engine();
		vkutil::VulkanManager &manager = engine.manager();

		vkutil::VertexInputDescriptionBuilder vertexBuilder(desc.get_stride());
		for (auto &pair : desc.get_attributes()) {
			vertexBuilder.push_attrib(atlas_to_vk_attribute(pair.first), pair.second);
		}

		vkutil::VertexInputDescription vertexInputDescription = vertexBuilder.value();

		vkutil::PipelineBuilder builder(manager);
		builder.set_vertex_description(vertexInputDescription)
			.set_color_format(engine.get_color_format())
			.set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL, engine.get_depth_format());

		std::vector<VkShaderModule> modules;

		for (auto &path : shaders) {
			std::filesystem::path file(path);

			if (!std::filesystem::exists(file)) {
				CORE_WARN("Shader: could not find file: {}", file);
				continue;
			}

			//VkShaderStageFlagBits shaderType = atlas_to_vk_shadertype(pair.second);

			std::filesystem::path ext = file.extension();

			bool success = false;
			VkShaderStageFlagBits shaderType{};

			VkShaderModule module{};
			if (ext == ".vert") {
				shaderType = VK_SHADER_STAGE_VERTEX_BIT;
				success = vkutil::load_glsl_shader_module(manager, file, VK_SHADER_STAGE_VERTEX_BIT, &module);
			}
			else if (ext == ".frag") {
				shaderType = VK_SHADER_STAGE_FRAGMENT_BIT;
				success = vkutil::load_glsl_shader_module(manager, file, VK_SHADER_STAGE_FRAGMENT_BIT, &module);
			}
			else {
				CORE_WARN("Shader: unknown extension: {}", ext);
				continue;
			}

			if (!success) {
				CORE_WARN("Shader: unknown error: {}", file);
				continue;
			}

			builder.add_shader_module(module, shaderType);
			modules.push_back(module);
		}

		//m_Shader = make_scope<vkutil::Shader>();
		vkutil::Shader shader{};
		builder.build(&shader);

		for (auto &module : modules) vkDestroyShaderModule(manager.device(), module, nullptr);

		m_Shader = engine.asset_manager().register_shader(shader);
	}

	Shader &Shader::operator=(Shader other)
	{
		m_Shader.swap(other.m_Shader);
		return *this;
	}

	Shader::~Shader()
	{
		if (auto shared = m_Shader.lock()) {
			Application::get_engine().asset_manager().deregister_shader(shared);
			vkDestroyPipeline(Application::get_engine().device(), shared->pipeline, nullptr);
		}
	}

	vkutil::Shader *Shader::get_native_shader()
	{
		if (auto shader = m_Shader.lock()) {
			return shader.get();
		}

		CORE_WARN("This shader was never created / or deleted!");
		return nullptr;
	}
}
