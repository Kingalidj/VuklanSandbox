#include "shader.h"
#include "application.h"
#include "descriptor.h"

#include "vk_pipeline.h"
#include "vk_engine.h"

#include "atl_vk_utils.h"

namespace vkutil {
	class VulkanShader {
	public:

		VulkanShader(Atlas::ShaderCreateInfo &info)
			:m_Descriptors(info.descriptors)
		{
			vkutil::VulkanEngine &engine = Atlas::Application::get_engine();
			vkutil::VulkanManager &manager = engine.manager();

			vkutil::VertexInputDescriptionBuilder vertexBuilder(info.vertexDescription.get_stride());
			for (auto &pair : info.vertexDescription.get_attributes()) {
				vertexBuilder.push_attrib(atlas_to_vk_attribute(pair.first), pair.second);
			}

			vkutil::VertexInputDescription vertexInputDescription = vertexBuilder.value();

			vkutil::PipelineBuilder builder(manager);
			builder.set_vertex_description(vertexInputDescription)
				.set_color_format(engine.get_color_format())
				.set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL, engine.get_depth_format());

			std::vector<VkDescriptorSetLayout> layouts;
			for (auto &d : info.descriptors) {
				layouts.push_back(d->get_native_descriptor()->layout);
			}

			if (layouts.size() != 0) {
				builder.set_descriptor_layouts(layouts);
			}

			std::vector<VkShaderModule> modules;

			for (auto &pair : info.shaderPaths) {
				std::filesystem::path file(pair.first);

				if (!std::filesystem::exists(file)) {
					CORE_WARN("Shader: could not find file: {}", file);
					continue;
				}

				VkShaderStageFlagBits shaderType = Atlas::atlas_to_vk_shaderstage(pair.second);

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

		VulkanShader &operator=(VulkanShader other)
		{
			m_Shader.swap(other.m_Shader);
			return *this;
		}

		~VulkanShader()
		{
			if (auto shared = m_Shader.lock()) {
				Atlas::Application::get_engine().asset_manager().deregister_shader(shared);
				vkDestroyPipeline(Atlas::Application::get_engine().device(), shared->pipeline, nullptr);
			}
		}

		void bind()
		{
			VkCommandBuffer cmd = Atlas::Application::get_engine().get_active_command_buffer();
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, get_native_shader()->pipeline);

			for (auto &d : m_Descriptors) {
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
					get_native_shader()->layout, 0, m_Descriptors.size(),
					&d->get_native_descriptor()->set, 0, nullptr);
			}
		}

		vkutil::Shader *get_native_shader()
		{
			if (auto shader = m_Shader.lock()) {
				return shader.get();
			}

			CORE_WARN("This shader was never created / or deleted!");
			return nullptr;
		}

	private:
		WeakRef<vkutil::Shader> m_Shader;
		std::vector<Ref<Atlas::Descriptor>> m_Descriptors;
	};
}

namespace Atlas {

	Shader::Shader(VertexDescription &desc, std::initializer_list<std::pair<const char *, ShaderStage>> shaders) {
		ShaderCreateInfo info{};
		info.vertexDescription = desc;
		info.shaderPaths = shaders;

		m_Shader = make_ref<vkutil::VulkanShader>(info);
	}

	Shader::Shader(VertexDescription &desc, std::initializer_list<std::pair<const char *, ShaderStage>> shaders,
		std::initializer_list<Ref<Descriptor>> descriptors) {

		ShaderCreateInfo info{};
		info.vertexDescription = desc;
		info.shaderPaths = shaders;
		info.descriptors = descriptors;

		m_Shader = make_ref<vkutil::VulkanShader>(info);
	}

	Shader::Shader(ShaderCreateInfo &info) {
		m_Shader = make_ref<vkutil::VulkanShader>(info);
	}


	//Shader::Shader(VertexDescription &desc, std::initializer_list<const char *> shaders) {
	//	ShaderCreateInfo info{};
	//	info.vertexDescription = desc;

	//	for (auto &s : shaders)
	//	{
	//		std::filesystem::path path(s);
	//		if (!std::filesystem::exists(path)) {
	//			CORE_WARN("Shader: could not find file: {}", path);
	//			continue;
	//		}

	//		auto ext = path.extension();
	//		ShaderStage type{};

	//		if (ext == ".vert") type = ShaderStage::VERTEX;
	//		else if (ext == ".frag") type = ShaderStage::FRAGMENT;
	//		else {
	//			CORE_WARN("Shader: unknown file extension: {}", ext);
	//			continue;
	//		}

	//		info.shaderPaths.push_back({ s, type });
	//	}

	//	m_Shader = make_ref<vkutil::VulkanShader>(info);
	//}

	void Shader::bind() {
		m_Shader->bind();
	}
}
