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

			std::vector<VkDescriptorSetLayout> layouts;
			for (auto &d : info.descriptors) {
				if (!d.is_init()) {
					CORE_WARN("Shader: descriptor was not initialized!");
					return;
				}

				layouts.push_back(d.get_native_descriptor()->layout);
			}

			vkutil::Shader shader{};

			//----------------------- compute ------------------------------------
			if (info.vertexDescription.size() == 0 && info.modules.size() == 1
				&& info.modules.at(0).get_stage() == Atlas::ShaderStage::COMPUTE) {

				auto &m = info.modules.at(0);

				VkShaderStageFlagBits shaderType = Atlas::atlas_to_vk_shaderstage(Atlas::ShaderStage::COMPUTE);
				std::vector<uint32_t> &buffer = m.get_data();

				VkShaderModule module{};
				bool success = vkutil::compile_shader_module(buffer.data(), (uint32_t)(buffer.size() * sizeof(uint32_t)),
					&module, manager.device());

				if (!success) {
					CORE_WARN("Shader: error while compiling shader: {}", m.get_file_path());
					return;
				}

				vkutil::create_compute_shader(manager, module, layouts, &shader.pipeline, &shader.layout);
				vkDestroyShaderModule(manager.device(), module, nullptr);
			}
			//----------------------- else ------------------------------------
			else {
				vkutil::VertexInputDescriptionBuilder vertexBuilder(info.vertexDescription.get_stride());
				for (auto &pair : info.vertexDescription.get_attributes()) {
					vertexBuilder.push_attrib(atlas_to_vk_attribute(pair.first), pair.second);
				}

				vkutil::VertexInputDescription vertexInputDescription = vertexBuilder.value();

				vkutil::PipelineBuilder builder(manager);

				if (layouts.size() != 0) {
					builder.set_descriptor_layouts(layouts);
				}

				builder.set_vertex_description(vertexInputDescription)
					.set_color_format(engine.get_color_format())
					.set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL, engine.get_depth_format());

				std::vector<VkShaderModule> modules;

				for (auto &m : info.modules) {
					VkShaderStageFlagBits shaderType = Atlas::atlas_to_vk_shaderstage(m.get_stage());

					std::vector<uint32_t> &buffer = m.get_data();

					VkShaderModule module{};
					bool success = vkutil::compile_shader_module(buffer.data(), (uint32_t)(buffer.size() * sizeof(uint32_t)),
						&module, manager.device());

					if (!success) {
						CORE_WARN("Shader: error while compiling shader: {}", m.get_file_path());
						continue;
					}

					builder.add_shader_module(module, shaderType);
					modules.push_back(module);
				}

				builder.build(&shader);
				for (auto &module : modules) vkDestroyShaderModule(manager.device(), module, nullptr);
			}


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
					get_native_shader()->layout, 0, (uint32_t)m_Descriptors.size(),
					&d.get_native_descriptor()->set, 0, nullptr);
			}
		}

		void update(uint32_t descSet, uint32_t binding, Atlas::DescriptorBinding descBinding) {
			CORE_ASSERT(descSet > m_Descriptors.size(), "descSet must an index into the descriptor sets");

			m_Descriptors.at(descSet).update(binding, descBinding);
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
		std::vector<Atlas::Descriptor> m_Descriptors;
	};
}

namespace Atlas {

	ShaderModule::ShaderModule(const char *path, ShaderStage stage, bool optimize)
		:m_Stage(stage), m_Path(path), m_Optimization(optimize)
	{
		auto filePath = std::filesystem::path(path);
		auto ext = filePath.extension();

		if (!std::filesystem::exists(filePath)) {
			CORE_WARN("Could not find file: {}", filePath);
		}
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			CORE_WARN("Could not open spv file: {}", filePath);
			return;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> data = std::vector<char>(fileSize);

		file.seekg(0);
		file.read((char *)data.data(), fileSize);
		file.close();

		if (ext == ".spv") {
			m_Data = make_ref<std::vector<uint32_t>>(fileSize / sizeof(uint32_t));
			memcpy(m_Data->data(), data.data(), data.size());
		}
		else {
			m_Data = make_ref<std::vector<uint32_t>>(vkutil::compile_glsl_to_spirv(path, atlas_to_vk_shaderstage(stage),
				(char *)data.data(), fileSize, optimize));
		}
	}

	Shader::Shader(ShaderCreateInfo &info) {
		m_Shader = make_ref<vkutil::VulkanShader>(info);
	}

	void Shader::bind() {
		m_Shader->bind();
	}

	std::optional<ShaderModule> load_shader_module(const char *path, ShaderStage stage, bool optimize)
	{
		ShaderModule module;
		module.m_Path = path;
		module.m_Stage = stage;
		module.m_Optimization = optimize;

		auto filePath = std::filesystem::path(path);
		auto ext = filePath.extension();

		if (!std::filesystem::exists(filePath)) {
			return std::nullopt;
		}
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			return std::nullopt;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> data = std::vector<char>(fileSize);

		file.seekg(0);
		file.read((char *)data.data(), fileSize);
		file.close();

		if (ext == ".spv") {
			module.m_Data = make_ref<std::vector<uint32_t>>(fileSize / sizeof(uint32_t));
			memcpy(module.m_Data->data(), data.data(), data.size());
		}
		else {
			module.m_Data = make_ref<std::vector<uint32_t>>(vkutil::compile_glsl_to_spirv(path, atlas_to_vk_shaderstage(stage),
				(char *)data.data(), fileSize, optimize));
		}

		return module;
	}

}
