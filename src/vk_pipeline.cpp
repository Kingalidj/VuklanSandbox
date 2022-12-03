#include "vk_pipeline.h"
#include "vk_scene.h"
#include "vk_manager.h"

#include <shaderc/shaderc.hpp>

#include <spirv_cross/spirv_reflect.hpp>
#include <spirv_cross/spirv_cross.hpp>

namespace vkutil {

	VkPipelineLayout PipelineLayoutCache::create_pipeline_layout(VkPipelineLayoutCreateInfo &info)
	{
		CORE_ASSERT(m_Device, "PipelineLayoutCache is not initialized");

		PipelineLayoutInfo layoutInfo;

		for (uint32_t i = 0; i < info.setLayoutCount; i++) {
			layoutInfo.m_Layouts.push_back(info.pSetLayouts[i]);
		}

		auto it = m_LayoutCache.find(layoutInfo);

		if (it != m_LayoutCache.end()) return (*it).second;
		else {
			VkPipelineLayout layout;
			vkCreatePipelineLayout(m_Device, &info, nullptr, &layout);

			m_LayoutCache[layoutInfo] = layout;
			return layout;
		}
	}

	void PipelineLayoutCache::cleanup()
	{
		for (auto &pair : m_LayoutCache) {
			vkDestroyPipelineLayout(m_Device, pair.second, nullptr);
		}
	}

	bool PipelineLayoutCache::PipelineLayoutInfo::operator==(const PipelineLayoutInfo &other) const {

		if (other.m_Layouts.size() != m_Layouts.size()) return false;

		for (uint32_t i = 0; i < m_Layouts.size(); i++) {
			if (other.m_Layouts[i] != m_Layouts[i]) return false;
		}

		return true;
	}

	size_t PipelineLayoutCache::PipelineLayoutInfo::hash() const {
		using std::size_t;
		using std::hash;

		size_t result = hash<size_t>()(m_Layouts.size());

		for (const VkDescriptorSetLayout &l : m_Layouts) {
			result ^= hash<size_t>()((uint64_t)l);
		}

		return result;
	}

	PipelineBuilder::PipelineBuilder(VulkanManager &manager)
	{
		m_Device = manager.device();
		m_LayoutCache = &manager.get_pipeline_layout_cache();
		m_VertexInputInfo = vkinit::vertex_input_state_create_info();
	}

	PipelineBuilder &PipelineBuilder::set_color_format(VkFormat format)
	{
		m_ColorFormat = format;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::add_shader_module(VkShaderModule shaderModule, VkShaderStageFlagBits shaderType)
	{
		m_ShaderStageInfo.push_back(vkinit::pipeline_shader_stage_create_info(shaderType, shaderModule));
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_renderpass(VkRenderPass renderpass)
	{
		m_RenderPass = renderpass;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_vertex_description(
		std::vector<VkVertexInputAttributeDescription> &attributes,
		std::vector<VkVertexInputBindingDescription> &bindings) {
		m_VertexInputInfo.pVertexAttributeDescriptions = attributes.data();
		m_VertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
		m_VertexInputInfo.pVertexBindingDescriptions = bindings.data();
		m_VertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindings.size();
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_vertex_description(VertexInputDescription &desc)
	{
		return set_vertex_description(desc.attributes, desc.bindings);
	}

	PipelineBuilder &PipelineBuilder::set_vertex_description(
		VkVertexInputAttributeDescription *pAttributes, uint32_t attributesCount,
		VkVertexInputBindingDescription *pBindings, uint32_t bindingCount)
	{
		m_VertexInputInfo.pVertexAttributeDescriptions = pAttributes;
		m_VertexInputInfo.vertexAttributeDescriptionCount = attributesCount;
		m_VertexInputInfo.pVertexBindingDescriptions = pBindings;
		m_VertexInputInfo.vertexBindingDescriptionCount = bindingCount;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_depth_stencil(bool depthTest, bool depthWrite, VkCompareOp compareOp, VkFormat depthFormat)
	{
		m_EnableDepthStencil = true;
		m_DepthStencil = vkinit::depth_stencil_create_info(depthTest, depthWrite, compareOp);
		m_DepthFormat = depthFormat;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_descriptor_layouts(std::vector<VkDescriptorSetLayout> layouts)
	{
		//m_DescriptorSetLayout = std::vector<VkDescriptorSetLayout>(layouts);
		m_DescriptorSetLayout = layouts;
		return *this;
	}

	bool PipelineBuilder::build(VkPipeline *pipeline, VkPipelineLayout *pipelineLayout)
	{

		VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
		layoutInfo.pSetLayouts = m_DescriptorSetLayout.data();
		layoutInfo.setLayoutCount = (uint32_t)m_DescriptorSetLayout.size();

		VkPipelineLayout layout = m_LayoutCache->create_pipeline_layout(layoutInfo);

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		auto colorBlendAttachment = vkinit::color_blend_attachment_state();
		auto inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		auto rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
		auto multisampling = vkinit::multisampling_state_create_info();

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;

		colorBlending.logicOpEnable = VK_FALSE;
		//colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkFormat color_rendering_format = m_ColorFormat;

		VkPipelineRenderingCreateInfoKHR pipeline_create{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
		pipeline_create.pNext = VK_NULL_HANDLE;
		pipeline_create.colorAttachmentCount = 1;
		pipeline_create.pColorAttachmentFormats = &color_rendering_format;
		pipeline_create.depthAttachmentFormat = m_DepthFormat;
		//pipeline_create.stencilAttachmentFormat = 0;

		VkGraphicsPipelineCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.pNext = &pipeline_create;

		createInfo.stageCount = (uint32_t)m_ShaderStageInfo.size();
		createInfo.pStages = m_ShaderStageInfo.data();
		createInfo.pVertexInputState = &m_VertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssembly;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizer;
		createInfo.pMultisampleState = &multisampling;
		createInfo.pColorBlendState = &colorBlending;
		createInfo.layout = layout;
		createInfo.renderPass = m_RenderPass;
		createInfo.subpass = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (m_EnableDepthStencil)
			createInfo.pDepthStencilState = &m_DepthStencil;

		VkPipelineDynamicStateCreateInfo dynStateInfo{};
		VkDynamicState dynStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynStateInfo.pNext = nullptr;
		dynStateInfo.pDynamicStates = dynStates;
		dynStateInfo.dynamicStateCount = 2;

		createInfo.pDynamicState = &dynStateInfo;

		*pipelineLayout = layout;
		auto res = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &createInfo, nullptr, pipeline);

		return res == VK_SUCCESS;

	}

	bool PipelineBuilder::build(VkPipeline *pipeline)
	{
		VkPipelineLayout layout;
		return build(pipeline, &layout);
	}

	bool PipelineBuilder::build(Shader *shader)
	{
		return build(&shader->pipeline, &shader->layout);
	}

	void reflect(std::vector<uint32_t> buffer) {

		spirv_cross::Compiler compiler(buffer);

		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		CORE_TRACE("{0} uniform buffers", resources.uniform_buffers.size());
		CORE_TRACE("{0} resources", resources.sampled_images.size());

		CORE_TRACE("Uniform buffers:");
		for (const auto &resource : resources.uniform_buffers) {
			const auto &bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			int memberCount = bufferType.member_types.size();

			CORE_TRACE("  {0}", resource.name);
			CORE_TRACE("    Size = {0}", bufferSize);
			CORE_TRACE("    Binding = {0}", binding);
			CORE_TRACE("    Members = {0}", memberCount);
		}


	}

	bool compile_shader_module(uint32_t *buffer, uint32_t byteSize,
		VkShaderModule *outShaderModule,
		const VkDevice device) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;

		createInfo.codeSize = byteSize;
		createInfo.pCode = buffer;

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
			VK_SUCCESS) {
			return false;
		}

		*outShaderModule = shaderModule;
		return true;
	}

	bool load_spirv_shader_module(const char *filePath,
		VkShaderModule *outShaderModule,
		const VkDevice device) {
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			CORE_WARN("Could not open spv file: {}", filePath);
			return false;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read((char *)buffer.data(), fileSize);
		file.close();

		if (!compile_shader_module(buffer.data(), buffer.size() * sizeof(uint32_t),
			outShaderModule, device)) {
			CORE_WARN("Could not compile shader: {}", filePath);
			return false;
		}

		return true;
	}

	std::vector<uint32_t> compile_glsl_to_spirv(const std::string &source_name,
		VkShaderStageFlagBits stage, const char *source, size_t sourceSize,
		bool optimize)
	{

		shaderc_shader_kind kind{};

		switch (stage) {
		case VK_SHADER_STAGE_VERTEX_BIT:
			kind = shaderc_vertex_shader;
			break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			kind = shaderc_fragment_shader;
			break;
		default:
			CORE_WARN("unknown extension for file: {}", source_name);
			return {};
		}

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan,
			shaderc_env_version_vulkan_1_3);

		if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

		shaderc::SpvCompilationResult module =
			compiler.CompileGlslToSpv(source, sourceSize, kind, source_name.c_str(), options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			CORE_WARN(module.GetErrorMessage());
		}

		return { module.cbegin(), module.cend() };
	}

	bool load_glsl_shader_module(const VulkanManager &manager, std::filesystem::path filePath,
		VkShaderStageFlagBits type,
		VkShaderModule *outShaderModule) {

		shaderc_shader_kind kind{};

		switch (type) {
		case VK_SHADER_STAGE_VERTEX_BIT:
			kind = shaderc_vertex_shader;
			break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			kind = shaderc_fragment_shader;
			break;
		default:
			CORE_WARN("unknown extension for file: {}", filePath);
			return false;
		}

		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			CORE_WARN("Could not open file: {}", filePath);
			return false;
		}

		size_t fileSize = (size_t)file.tellg();
		std::string source;
		source.resize(fileSize / sizeof(char));

		file.seekg(0);
		file.read((char *)source.data(), fileSize);
		file.close();


		std::vector<uint32_t> buffer = compile_glsl_to_spirv(filePath.u8string(), type, source.data(), fileSize);

		if (!compile_shader_module(buffer.data(), buffer.size() * sizeof(uint32_t),
			outShaderModule, manager.device())) {
			CORE_WARN("Could not compile shader: {}", filePath);
			return false;
		}

		return true;
	}

	bool load_glsl_shader_module(const VulkanManager &manager, std::filesystem::path filePath, VkShaderModule *outShaderModule) {
		auto ext = filePath.extension();

		VkShaderStageFlagBits type;
		if (ext == ".vert") {
			type = VK_SHADER_STAGE_VERTEX_BIT;
		}
		else if (ext == ".frag") {
			type = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else {
			CORE_WARN("unknown extension for file: {}", filePath);
			return false;
		}

		return load_glsl_shader_module(manager, filePath, type, outShaderModule);
	}


} //namespace vkutil
